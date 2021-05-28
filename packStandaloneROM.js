// Run this script with Node in the same directory as the ROM
// Generates an fs_standalone.cpp file that can be used for standalone builds
const fs = require("fs");
let out = fs.openSync("fs_standalone.cpp", "w")
fs.writeSync(out, '#include "FileEntry.hpp"\nFileEntry FileEntry::NULL_ENTRY = {false, NULL, {}};\n')
function writeDir(name, path) {
    console.log("Reading directory " + path);
    let data = "";
    let count = 0;
    for (var f of fs.readdirSync(path)) {
        if (f == "." || f == ".." || f == ".DS_Store" || f == "desktop.ini" || f == "packStandaloneROM.js") continue;
        const isDir = fs.lstatSync(path + "/" + f).isDirectory();
        data += `    {"${f}", `;
        if (isDir) {
            const c = writeDir(name + "_" + f, path + "/" + f);
            data += `{true, NULL, dir_${name}_${f}, ${c}}},\n`;
        } else {
            console.log("Reading file " + path + "/" + f);
            const d = fs.readFileSync(path + "/" + f);
            let s = "";
            for (let b of d) s += "\\x" + (b < 16 ? "0" : "") + b.toString(16);
            data += `{false, "${s}", NULL, ${d.length}}},\n`;
        }
        count++;
    }
    fs.writeSync(out, "static DirEntry dir_" + name + "[] = {\n" + data + "    {NULL, FileEntry::NULL_ENTRY}\n};\n");
    return count;
}
const c = writeDir("rom", "rom");
fs.writeSync(out, "FileEntry standaloneROM = {true, NULL, dir_rom, " + c + "};\n\nconst char * standaloneBIOS = ");
console.log("Reading BIOS");
const bios = fs.readFileSync("bios.lua");
let s = "\"";
for (let b of bios) s += "\\x" + (b < 16 ? "0" : "") + b.toString(16);
fs.writeSync(out, s + "\";\nsize_t standaloneBIOSSize = " + bios.length + ";");
fs.closeSync(out);
