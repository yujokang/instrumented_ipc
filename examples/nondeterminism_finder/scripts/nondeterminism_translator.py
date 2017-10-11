from subprocess import Popen, PIPE
from os import pardir, sep
from os.path import basename, dirname, abspath, join
from elftools.elf.elffile import ELFFile, StringTableSection

SCRIPT_DIR = dirname(abspath(__file__))

UNKNOWN_MARKER = "??"

def get_addr(section):
	return section.header["sh_addr"]
def get_size(section):
	return section.header["sh_size"]

class LocationSearcher:
	__binary_path = None
	__binary_file = None
	__sections = None
	def __init__(self, binary_path):
		self.__binary_path = binary_path
		self.__binary_file = open(binary_path, "rb")
		elf = ELFFile(self.__binary_file)
		self.__sections = []
		for section in elf.iter_sections():
			addr = get_addr(section)
			size = get_size(section)
			end = addr + size
			self.__sections.append((addr, end, section))
		self.__sections.sort()
	def __impossible(self, address):
		finder = Popen(["addr2line", "-e", self.__binary_path,
				str(address)],
				stdout = PIPE, stderr = PIPE)
		out, err = finder.communicate()
		out_lines = out.decode().rstrip().split(":")
		for out_line in out_lines:
			if not out_line.startswith(UNKNOWN_MARKER):
				return False
		return True
	def __find_string(self, name_location, start, end):
		if start >= end:
			return None
		middle = (start + end) // 2
		middle_value = self.__sections[middle]
		middle_start, middle_end, middle_section = middle_value
		if name_location < middle_start:
			return self.__find_string(name_location, start, middle)
		elif name_location >= middle_end:
			return self.__find_string(name_location, middle + 1,
							end)
		else:
			offset = name_location - middle_start
			if isinstance(middle_section, StringTableSection):
				raw_name = middle_section.get_string(offset)
				return raw_name.decode()
			else:
				running_name = ""
				data = middle_section.data()
				for raw_byte in data[offset :]:
					if raw_byte == 0:
						return running_name
					else:
						running_name += chr(raw_byte)
	def find_string(self, address, name_location):
		if self.__impossible(address):
			return None
		return self.__find_string(name_location, 0,
						len(self.__sections))
	def close(self):
		self.__binary_file.close()

DETERMINISTIC_LINE = "deterministic"
PAIR_DIV = "/"

def interpret_result(binaries, result_lines):
	if len(result_lines) == 1:
		return []
	result_line = result_lines[1]
	if result_line == DETERMINISTIC_LINE:
		return []
	else:
		branch_pair, _, first_pair, second_pair = result_line.split()
		names = []
		for pair in [branch_pair, first_pair, second_pair]:
			address, name_location = map(lambda hex_str:
							int(hex_str, 0x10),
							pair.split(PAIR_DIV))
			for binary in binaries:
				name = binary.find_string(address,
								name_location)
				if name is not None:
					names.append(name)
		return names

TRACKER_PATH = join(SCRIPT_DIR, pardir + sep + "nondeterminism-tracker")
LIST_DELIM = ","

def run_test(test_args, ignores, program_list_str, binaries):
	args = [TRACKER_PATH]
	if program_list_str is not None:
		args += ["-p", program_list_str]
	if len(ignores) > 0:
		args += ["-i", LIST_DELIM.join(ignores)]
	args += test_args
	runner = Popen(args, stdout = PIPE, stderr = PIPE)
	out, err = runner.communicate()
	result_lines = out.decode().rstrip().split("\n")
	return interpret_result(binaries, result_lines)

def keep_trying(test_args, binary_list_str):
	ignores = set()
	have_something = True
	if binary_list_str is not None:
		binary_paths = binary_list_str.split(LIST_DELIM)
		program_list = list(map(basename, binary_paths))
		program_list_str = LIST_DELIM.join(program_list)
	else:
		binary_paths = [test_args[0]]
		program_list_str = None
	binaries = list(map(LocationSearcher, binary_paths))
	while have_something:
		functions = run_test(test_args, ignores, program_list_str,
					binaries)
		if len(functions) == 0:
			have_something = False
		else:
			for function in functions:
				if function in ignores:
					print("Could not ignore %s" %
						(function))
			ignores.update(functions)
	for binary in binaries:
		binary.close()
	return ignores

if __name__ == "__main__":
	from sys import argv

	FLAG_START = '-'
	BINARY_LIST_FLAG = FLAG_START + "p"
	def print_usage():
		print("Usage: " + argv[0] + \
			" (-p [optional list of programs to watch]) " + \
			"[test command and arguments...]")
		exit(-1)

	binary_list_str = None
	test_arg_start = None
	arg_i = 1
	while arg_i < len(argv) and test_arg_start is None:
		arg = argv[arg_i]
		if len(arg) > 0:
			if arg[0] == FLAG_START:
				arg_i += 1
				if arg_i >= len(argv):
					print_usage()
				if arg == BINARY_LIST_FLAG:
					binary_list_str = argv[arg_i]
					
			else:
				test_arg_start = arg_i
		arg_i += 1
	if test_arg_start is None:
		print_usage()
	ignores = keep_trying(argv[test_arg_start : ], binary_list_str)
	print("You should ignore: %s" % (LIST_DELIM.join(ignores)))
