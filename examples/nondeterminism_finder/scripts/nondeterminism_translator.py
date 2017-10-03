from subprocess import Popen, PIPE
from os import pardir, sep
from os.path import basename, dirname, abspath, join

SCRIPT_DIR = dirname(abspath(__file__))

UNKNOWN_FILE = "??"

def get_line(binary, address):
	finder = Popen(["addr2line", "-e", binary, address], stdout = PIPE,
			stderr = PIPE)
	out, err = finder.communicate()
	out_line = out.decode().rstrip()
	if out_line.startswith(UNKNOWN_FILE):
		return None
	else:
		source_path, line_num_str = out_line.split(":")
		return source_path, int(line_num_str)

def get_source_name(binary, address):
	line_result = get_line(binary, address)
	if line_result is None:
		return None
	else:
		source_path = line_result[0]
		return basename(source_path)

DETERMINISTIC_LINE = "deterministic"

def interpret_result(binary, result_lines):
	if len(result_lines) == 1:
		return []
	result_line = result_lines[1]
	if result_line == DETERMINISTIC_LINE:
		return []
	else:
		branch, _, first, second = result_line.split()
		source_names = []
		for address in [branch, first, second]:
			source_name = get_source_name(binary, address)
			if source_name is not None:
				source_names.append(source_name)
		return source_names

TRACKER_PATH = join(SCRIPT_DIR, pardir + sep + "nondeterminism-tracker")
LIST_DELIM = ","

def run_test(test_args, ignores, program_list_str):
	args = [TRACKER_PATH]
	if program_list_str is not None:
		args += ["-p", program_list_str]
	if len(ignores) > 0:
		args += ["-s", LIST_DELIM.join(ignores)]
	args += test_args
	runner = Popen(args, stdout = PIPE, stderr = PIPE)
	out, err = runner.communicate()
	result_lines = out.decode().rstrip().split("\n")
	binary = test_args[0]
	return interpret_result(binary, result_lines)

def keep_trying(test_args, program_list_str):
	ignores = set()
	have_something = True
	while have_something:
		source_names = run_test(test_args, ignores, program_list_str)
		if len(source_names) == 0:
			have_something = False
		else:
			ignores.update(source_names)
	return ignores

if __name__ == "__main__":
	from sys import argv

	FLAG_START = '-'
	PROGRAM_LIST_FLAG = FLAG_START + "p"
	def print_usage():
		print("Usage: " + argv[0] + \
			" (-p [optional list of programs to watch]) " + \
			"[test command and arguments...]")
		exit(-1)

	program_list_str = None
	test_arg_start = None
	arg_i = 1
	while arg_i < len(argv) and test_arg_start is None:
		arg = argv[arg_i]
		if len(arg) > 0:
			if arg[0] == FLAG_START:
				if arg_i == len(argv) - 1:
					print_usage()
				if arg == PROGRAM_LIST_FLAG:
					program_list_str = argv[arg_i + 1]
					
			else:
				test_arg_start = arg_i
		arg_i += 1
	if test_arg_start is None:
		print_usage()
	ignores = keep_trying(argv[test_arg_start : ], program_list_str)
	print("You should ignore: %s" % (LIST_DELIM.join(ignores)))
