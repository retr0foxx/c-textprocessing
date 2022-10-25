"""
    Creates copies of the inputted text file into multiple file with different encodings
    that are supported by the textreader
"""

import sys;
import os;

def string_rindex(string, string1):
    try:
        result = string.rindex(string1);
    except ValueError:
        return -1;

    return result;

def get_path_extension(path):
    last_dot_index = path.rindex('.');
    if (last_dot_index < max(string_rindex(path, '\\'), string_rindex(path, '/'))):
        return None;

    return path[last_dot_index+1:];

argv_start = 1;

supported_encodings = {
    "utf-8",
    "utf-16be",
    "utf-16le",
    "ansi"
};

if (len(sys.argv) <= argv_start+1 or sys.argv[argv_start] == ''):
    print(f"Usage:\n\t{os.path.basename(__file__)} <converted filename> <converted file encoding>\n");
    print("Supported encodings:\n\t", supported_encodings, sep="");
    exit(1);

file = sys.argv[argv_start];
encoding = sys.argv[argv_start+1];

if (encoding not in supported_encodings):
    print(f"Unsupported encoding \'{encoding}\'");
    print("Supported encodings:\n\t", supported_encodings, sep="");

file_path_prefix = "";
if (not (len(file) > 1 and file[1] == ':' and file[0].isupper())):
    file_path_prefix = os.getcwd() + '/';

file_path = file_path_prefix + file;

if (not os.path.isfile(file_path)):
    print(f"'{file_path}' is not a file!");
    exit(1);

with open(file_path, "rb") as f:
    file_content = f.read();

file_content = file_content.decode(encoding);

# (filename, encoding, binary prefix?)
converted_encodings = (
    ("utf-16-be", "utf-16be"),
    ("utf-16-be-bom", "utf-16be", b"\xfe\xff"),

    ("utf-16-le", "utf-16le"),
    ("utf-16-le-bom", "utf-16le", b"\xff\xfe"),
    
    ("utf-8", "utf-8"),
    ("utf-8-bom", "utf-8", b"\xEF\xBB\xBF"),
);

file_path_extension = get_path_extension(file_path);

for converted_encoding in converted_encodings:
    converted_text = b"";
    if (len(converted_encoding) > 2):
        converted_text = converted_encoding[2];

    converted_text += file_content.encode(converted_encoding[1]);
    with open(converted_encoding[0] + '.' + file_path_extension, "wb") as f:
        f.write(converted_text);