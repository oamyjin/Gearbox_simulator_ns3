import sys
from optparse import OptionParser
import matplotlib.pyplot as plt


def readPart(filePath, size=1024, encoding="utf-8"):
	with open(filePath, "r", encoding=encoding) as f:
		while True:
			part = f.read(size)
			if part:
				yield part
			else:
				return None


if __name__ == "__main__":
	parser = OptionParser()
	parser.add_option("-t", "--traffic", dest = "traffic", help = "the traffic file name")
	parser.add_option("-f", "--fileName", dest = "fileName", help = "the file name", default = "rankVtDifference.dat")
	parser.add_option("-d", "--drop", dest = "drop", help = "the file name", default = "drop.dat")
	parser.add_option("-o", "--output", dest = "output", help = "the output file", default = "rankDiffDistribution.dat")
	options,args = parser.parse_args()

	total_size = 0
	maxDiff = 1500

	# count the total traffic size
	if options.traffic != None:
		file = open(options.traffic, "r")
		lines = file.readlines()
		l = 0
		for line in lines:
			l += 1
			if l == 1:
				continue
			else:
				line = line.strip("\n")
				line = line.split()
				total_size = total_size + int(line[3])
				print(int(line[3]))
		print("total size:", total_size)
		sys.exit(0)


	# count the number for each rank-vt difference value
	filePath = options.fileName
	size = 81920000  # 每次读取指定大小的内容到内存
	encoding = 'utf8'
	i = 1
	for part in readPart(filePath, size, encoding):
		i_str = str(i)
		i = i + 1

		# process sub input
		output = options.output
		ofile = open(output, "w")

		count = []
		for d in range(maxDiff):
			count.append(0)
		print(len(count))

		print("maxDiff:", maxDiff)

		'''with open(input, "r") as f:
			for line in part.readlines():
				line = line.strip("\n")
				line = line.split()
				# count and record
				for i in range(maxDiff):
					#print(i, " ", line.count(str(i)))
					count[i] = count[i] + line.count(str(i))
		'''
		part = part.strip("\n")
		part = part.split()
		# count and record
		for i in range(maxDiff):
			count[i] = count[i] + part.count(str(i))


		# write to file
		for i in range(maxDiff):
			ofile.write("%d %d\n" % (i, count[i]))

		# plot
		point = []
		y = 100
		for i in range(y):
			point.append(count[i])
		plt.plot(range(y), point)  # b代表blue颜色  -代表直线
		plt.title('rank-vt Difference Distribution')
		plt.xlabel('rank-vt Difference Value')
		plt.ylabel('Count')
		plt.show()

	ofile.close()

# python count_size.py -t traffic.txt
# python count_size.py -t traffic_128_0.3_500_1.txt
# python count_size.py -f rankVtDifference.dat