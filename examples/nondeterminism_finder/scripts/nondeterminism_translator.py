from subprocess import Popen, PIPE
from os import pardir, sep
from os.path import basename, dirname, abspath, join

SCRIPT_DIR = dirname(abspath(__file__))

UNKNOWN_MARKER = "??"

def maybe_get_location(original):
	if UNKNOWN_MARKER in original:
		return None
	return original

FUNCTION_LINE_NUMBER = 0
SOURCE_LINE_NUMBER = 1

def get_source(binary, address):
	finder = Popen(["addr2line", "-e", binary, "-f", address], stdout = PIPE,
			stderr = PIPE)
	out, err = finder.communicate()
	out_lines = out.decode().rstrip().split("\n")
	function_name = maybe_get_location(out_lines[FUNCTION_LINE_NUMBER])
	source_path, line_num_str = \
		map(maybe_get_location,
			out_lines[SOURCE_LINE_NUMBER].split(":"))
	if line_num_str is None:
		line_num = None
	else:
		line_num = int(line_num_str.split()[0])
	return function_name, source_path, line_num

def get_function(binary, address):
	function_name, source_path, line_num  = get_source(binary, address)
	return function_name

DETERMINISTIC_LINE = "deterministic"

def interpret_result(binaries, result_lines):
	if len(result_lines) == 1:
		return []
	result_line = result_lines[1]
	if result_line == DETERMINISTIC_LINE:
		return []
	else:
		branch, _, first, second = result_line.split()
		functions = []
		for address in [branch, first, second]:
			for binary in binaries:
				function = get_function(binary, address)
				if function is not None:
					functions.append(function)
		return functions

TRACKER_PATH = join(SCRIPT_DIR, pardir + sep + "nondeterminism-tracker")
LIST_DELIM = ","

def run_test(test_args, ignores, binary_list_str):
	args = [TRACKER_PATH]
	if binary_list_str is not None:
		binaries = binary_list_str.split(LIST_DELIM)
		program_list = list(map(basename, binaries))
		program_list_str = LIST_DELIM.join(program_list)
		args += ["-p", program_list_str]
	else:
		binaries = [test_args[0]]
	if len(ignores) > 0:
		args += ["-i", LIST_DELIM.join(ignores)]
	args += test_args
	print(" ".join(args))
	runner = Popen(args, stdout = PIPE, stderr = PIPE)
	out, err = runner.communicate()
	result_lines = out.decode().rstrip().split("\n")
	print(result_lines)
	return interpret_result(binaries, result_lines)

def keep_trying(test_args, binary_list_str):
	ignores = set()
	have_something = True
	while have_something:
		functions = run_test(test_args, ignores, binary_list_str)
		if len(functions) == 0:
			have_something = False
		else:
			for function in functions:
				if function in ignores:
					print("Already have %s" % (function))
			ignores.update(functions)
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
