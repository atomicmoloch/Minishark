import csv
import sys

###################################################################
#
# Function: generate_c_database
#
# Takes: filenames inputFile, outputFile
#        Expecting inputFile to be a csv spreadsheet of MAC address prefixes
#        with prefixes in column 1 and manufacturer name in column 2
#        as may be obtained up-to-date from maclookup.app
#
# Returns: Nothing
#
###################################################################

def generate_c_database(inputFile, outputFile):
    prefixes = []
    manufacturers = ["Unknown"]
    man_dict = {"Unknown": 0}

    with open(inputFile, newline='', encoding='utf-8') as infile:
        try:
            reader = csv.reader(infile)

            for row in reader:
                if not row:
                    continue
                prefix = row[0].strip()
                manufacturer = row[1].strip().replace("?", "").replace("\"", "") if len(row) > 1 else "Unknown"

                if manufacturer not in man_dict:
                    man_dict[manufacturer] = len(manufacturers)
                    manufacturers.append(manufacturer)

                man_id = man_dict[manufacturer]

                hex_bytes = prefix.split(":")

                length = len(hex_bytes)
                prefix_val = 0
                for b in hex_bytes:
                    prefix_val = (prefix_val << 8) | int(b, 16)

                prefixes.append((prefix_val, length, man_id))
        except:
            print("Error: Problem with " + inputFile)
            return

    with open(outputFile, "w", encoding="utf-8") as cfile:
        try:
            cfile.write("#include <stdint.h>\n")
            cfile.write("#include <stddef.h>\n")
            cfile.write("#include \"mac_database.h\"\n\n")
            cfile.write("/* Auto-generated OUI/MA-M/MA-S lookup tables */\n\n")

            # prefix table entry type
         #   cfile.write("typedef struct {\n")
         #   cfile.write("   uint64_t prefix;\n")
         #   cfile.write("   uint8_t length;\n")
         #   cfile.write("   uint16_t man_id;\n")
         #   cfile.write("} oui_entry_t;\n\n")

            cfile.write("const char *manufacturers[] = {\n")
            for m in manufacturers:
                cfile.write(f'  "{m}",\n')
            cfile.write("};\n\n")

            cfile.write("const oui_entry_t oui_table[] = {\n")
            for prefix, length, man_id in prefixes:
                cfile.write(f"  {{0x{prefix:X}, {length}, {man_id}}},\n")
            cfile.write("};\n\n")

            cfile.write("const size_t oui_table_length = (sizeof(oui_table)/sizeof(oui_entry_t));")
        except:
            print("Error: Problem with " + outputFile)
            return

    print("Complete")


if __name__ == "__main__":
    if len(sys.argv) >= 3:
        generate_c_database(sys.argv[1], sys.argv[2])
    else:
        print("Usage: convert_c <INPUT CSV> <C OUTPUT FILE>")
