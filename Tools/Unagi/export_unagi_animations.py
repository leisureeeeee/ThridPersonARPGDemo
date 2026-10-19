"""3ds Max batch entry point. Requires a licensed Max installation with pymxs.

Only run in a separate 3dsmaxbatch process via Export-UnagiAnimations.ps1.
The script loads source scenes without saving them and never replaces FBX files.
"""

import hashlib
import json
import os
from pathlib import Path

from pymxs import runtime as rt


def configure(name, value):
    result = rt.FBXExporterSetParam(name, value)
    if str(result).lower() != "ok":
        raise RuntimeError("FBX exporter rejected parameter {}: {}".format(name, result))


def main():
    config = json.loads(Path(os.environ["UNAGI_EXPORT_CONFIG"]).read_text(encoding="utf-8-sig"))
    report_path = Path(config["report_path"])
    report = {"status": "running", "clips": [], "max_version": str(rt.maxVersion())}

    def write_report():
        report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    write_report()
    # This affects this newly launched batch process, never the user's open Max.
    rt.setSilentMode(True)
    for job in config["jobs"]:
        source = Path(job["source"])
        target = Path(job["output"])
        entry = {"source": str(source), "output": str(target)}
        report["clips"].append(entry)
        if target.exists():
            entry["status"] = "skipped_existing"
            write_report()
            continue
        try:
            source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
            rt.resetMaxFile(rt.name("noPrompt"))
            if not rt.loadMaxFile(str(source), quiet=True, useFileUnits=True,
                                  missingDLLsAction=rt.name("abort"),
                                  missingXRefsAction=rt.name("abort"),
                                  missingExtFilesAction=rt.name("logmsg")):
                raise RuntimeError("Could not load scene")
            rt.execute("pluginManager.loadClass FBXEXP")
            if str(rt.FBXExporterSetParam("ResetExport")).lower() != "ok":
                raise RuntimeError("Could not load FBX export preset")
            start = int(rt.animationRange.start.frame)
            end = int(rt.animationRange.end.frame)
            if end <= start:
                raise RuntimeError("Scene has no nonzero animation time range")
            for name, value in (
                ("Animation", True), ("BakeAnimation", True),
                ("BakeFrameStart", start), ("BakeFrameEnd", end),
                ("BakeFrameStep", 1), ("BakeResampleAnimation", True),
                ("Skin", True), ("Shape", False), ("Cameras", False),
                ("Lights", False), ("EmbedTextures", False),
                ("ASCII", False), ("GenerateLog", True),
            ):
                configure(name, value)
            target.parent.mkdir(parents=True, exist_ok=True)
            # Keep the complete rig and skinned meshes for reliable bone discovery
            # by UE. The UE stage disables mesh, material, and texture import.
            exported = rt.exportFile(str(target), rt.name("noPrompt"), selectedOnly=False, using=rt.FBXEXP)
            if not exported or not target.is_file() or target.stat().st_size == 0:
                raise RuntimeError("FBX exporter did not produce a nonempty file")
            if hashlib.sha256(source.read_bytes()).hexdigest() != source_hash:
                raise RuntimeError("Source scene changed on disk")
            entry.update(status="exported", start_frame=start, end_frame=end,
                         fps=int(rt.frameRate), source_sha256=source_hash,
                         output_bytes=target.stat().st_size)
        except Exception as error:
            entry.update(status="failed", error=str(error))
        write_report()
    report["status"] = "failed" if any(c["status"] == "failed" for c in report["clips"]) else "complete"
    write_report()
    if report["status"] != "complete":
        raise RuntimeError("Some MAX exports failed; see " + str(report_path))


if __name__ == "__main__":
    main()
