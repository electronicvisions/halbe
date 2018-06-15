import json
import argparse

def equal_value(x, y, path):
	if x == y:
		return True
	elif x == 0:
		if y == 0:
			return True
		else:
			return False
	elif x != 0:
		if path[1] == 'currents':
			if abs(x-y) < ((2.5e-6)/1024):
				return True
			else:
				print path
				return False
		elif path[1] == 'voltages' or path[1] == 'initials':
			if abs (x-y) < (1.8/1023):
				return True
			else:
				print path
				return False
		else:
			print "values not compared: error"
			return False
			
def compare (a, b, path=None, key="toplevel"):
	if path is None:
		path=[]
	path.append(key)
	keys_list_a = []
	keys_list_b = []
	for key in a:
		keys_list_a.append(key)
	for key in b:
		keys_list_b.append(key)
	
	if "values" in keys_list_a:
		if "width" in keys_list_b and "values" in keys_list_b:
			if len(a["values"]) == len(b["values"]) and a["width"] == b["width"]:
				for i in range(len(a["values"])):
						if isinstance(a["values"][i],list) and isinstance(b["values"][i], list):
							for s in range(len(a["values"][i])):
								if not equal_value(a["values"][i][s], b["values"][i][s], path):
									return "value error"
						else:
							if isinstance(a["values"][i],list) or isinstance(b["values"][i], list):
								print "apples and oranges!"
								print path
								return "value error"
							else:
								if not equal_value(a["values"][i], b["values"][i], path):
									return "value error"
		else:
			return "value error"
	elif set(keys_list_a) == set(keys_list_b):
		for key in a:
			path = path[:]
			if compare(a[key], b[key], path, key) == "value error":
				path.pop()
				print "value error in key: " +str(key)
				print "should:"
				print str(a[key]) #["values"]) + ", width: " + str(a[key]["width"])
				print "is:"
				print str(b[key]) #["values"]) + ", width: " + str(b[key]["width"])
				print "---------------------------------------"
			else:
				path.pop()
	else:
		print "Key error:"
		diff_a_minus_b = list(set(keys_list_a) -set(keys_list_b))
		diff_b_minus_a = list(set(keys_list_b) -set(keys_list_a))
		print "In input a but not in b:"
		print diff_a_minus_b
		print "In input b but not in a:"
		print diff_b_minus_a

if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('-i1', '--input-file1', help='Specify first JSON')
	parser.add_argument('-i2', '--input-file2', help='Specify second JSON')
	args = parser.parse_args()
	inf1 = args.input_file1
	inf2 = args.input_file2
    
	with open(inf1, 'r') as first, open(inf2, 'r') as second:
		data_1 = json.load(first)
		data_2 = json.load(second)

	compare(data_1, data_2)
