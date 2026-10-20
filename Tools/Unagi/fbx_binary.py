"""Small binary FBX reader/writer that preserves original property bytes.

Used for object-level extraction, without decoding or re-encoding mesh arrays.
Supports the 32-bit and 64-bit binary node headers.
"""

from dataclasses import dataclass, field
import struct
import zlib


@dataclass
class Node:
    name: bytes
    properties: list[bytes] = field(default_factory=list)
    children: list = field(default_factory=list)
    terminated: bool = False

    def child(self, name):
        return next((n for n in self.children if n.name == name), None)


def value(raw):
    kind = chr(raw[0])
    formats = {"Y": "h", "C": "?", "I": "i", "F": "f", "D": "d", "L": "q"}
    if kind in formats:
        return struct.unpack_from("<" + formats[kind], raw, 1)[0]
    if kind in "SR":
        return raw[5:]
    raise ValueError("Not a scalar property: " + kind)


def scalar(item, kind):
    if kind == "S":
        data = item.encode() if isinstance(item, str) else item
        return b"S" + struct.pack("<I", len(data)) + data
    return kind.encode() + struct.pack("<" + {"I": "i", "L": "q"}[kind], item)


def array(raw):
    count, encoding, size = struct.unpack_from("<III", raw, 1)
    data = raw[13:13 + size]
    if encoding == 1:
        data = zlib.decompress(data)
    elif encoding != 0:
        raise ValueError("Unknown FBX array encoding")
    fmt = {"f": "f", "d": "d", "l": "q", "i": "i", "b": "?", "c": "b"}[chr(raw[0])]
    if len(data) != count * struct.calcsize(fmt):
        raise ValueError("Invalid FBX array length")
    return struct.unpack("<" + str(count) + fmt, data)


def read(data):
    if data[:23] != b"Kaydara FBX Binary  \x00\x1a\x00":
        raise ValueError("Expected a binary FBX")
    version = struct.unpack_from("<I", data, 23)[0]
    header = "<QQQB" if version >= 7500 else "<IIIB"
    header_size = struct.calcsize(header)

    def parse(offset):
        end, count, prop_size, name_size = struct.unpack_from(header, data, offset)
        if end == 0:
            if data[offset:offset + header_size] != bytes(header_size):
                raise ValueError("Malformed FBX node terminator")
            return None, offset + header_size
        if not offset < end <= len(data):
            raise ValueError("Invalid FBX node end offset")
        cursor = offset + header_size
        node = Node(data[cursor:cursor + name_size])
        cursor += name_size
        prop_end = cursor + prop_size
        for _ in range(count):
            start = cursor
            kind = chr(data[cursor])
            cursor += 1
            if kind in "YCIFDL":
                cursor += {"Y": 2, "C": 1, "I": 4, "F": 4, "D": 8, "L": 8}[kind]
            elif kind in "SR":
                size = struct.unpack_from("<I", data, cursor)[0]
                cursor += 4 + size
            elif kind in "fdlibc":
                size = struct.unpack_from("<III", data, cursor)[2]
                cursor += 12 + size
            else:
                raise ValueError("Unsupported FBX property: " + kind)
            node.properties.append(data[start:cursor])
        if cursor != prop_end:
            raise ValueError("FBX property length mismatch")
        while cursor < end:
            child, cursor = parse(cursor)
            if child is None:
                node.terminated = True
                break
            node.children.append(child)
        if cursor != end:
            raise ValueError("FBX child length mismatch")
        return node, end

    nodes, cursor = [], 27
    while True:
        node, cursor = parse(cursor)
        if node is None:
            break
        nodes.append(node)
    return version, nodes, data[cursor:]


def write(version, nodes, footer):
    header = "<QQQB" if version >= 7500 else "<IIIB"
    header_size = struct.calcsize(header)
    output = bytearray(b"Kaydara FBX Binary  \x00\x1a\x00" + struct.pack("<I", version))

    def emit(node):
        start = len(output)
        output.extend(bytes(header_size))
        output.extend(node.name)
        props = b"".join(node.properties)
        output.extend(props)
        for child in node.children:
            emit(child)
        if node.children or node.terminated:
            output.extend(bytes(header_size))
        struct.pack_into(header, output, start, len(output), len(node.properties), len(props), len(node.name))

    for node in nodes:
        emit(node)
    output.extend(bytes(header_size))
    # Retain the original footer (readers consume the node tree via offsets).
    output.extend(footer)
    return bytes(output)


def fingerprint(node):
    """Content-only digest, excluding absolute node offsets."""
    import hashlib
    digest = hashlib.sha256()

    def visit(item):
        digest.update(struct.pack("<I", len(item.name)))
        digest.update(item.name)
        for prop in item.properties:
            digest.update(struct.pack("<I", len(prop)))
            digest.update(prop)
        digest.update(struct.pack("<I", len(item.children)))
        for child in item.children:
            visit(child)

    visit(node)
    return digest.hexdigest()
