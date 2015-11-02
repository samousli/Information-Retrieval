import os
import sys
import codecs

if __name__ == "__main__":
	print("This script parses a directory to create an index of the text they contain,")
	print("which in turn can be used to feed data into the inverted index algorithm")

	if (len(sys.argv) >= 2):
		path = sys.argv[1]
	else:
		print('Please insert the path which contains the source text')
		sys.exit()
	
	files = next(os.walk(path))[2]
	file_count = len(files)
	doc_id = 0
	# Read docs
	curr_dir = os.path.dirname(os.path.abspath(__file__))
	with codecs.open(curr_dir +'/input.txt', "w", encoding="utf-8") as output:
		output.write(str(file_count) + '\n')
		for file in files:
			
			with codecs.open(path + '/' + file, "r", encoding="utf-8") as f:
				s = f.read(-1)
				s = s.strip().replace("\r", " ").replace("\n", " ").replace("\t", " ")
				output.write(str(doc_id) + ' ' + s + '\n')
				doc_id += 1
				print(file)