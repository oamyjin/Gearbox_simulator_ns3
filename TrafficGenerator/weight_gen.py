import random
from optparse import OptionParser

if __name__ == "__main__":
    port = 80
    parser = OptionParser()
    parser.add_option("-n", "--number", dest="n", help="number of weight values", default="10")
    parser.add_option("-r", "--range", dest="range", help="range of weight values", default="16")
    options, args = parser.parse_args()

    if not options.n:
        print("please use -n to enter number of weights")
        sys.exit(0)

    n = int(options.n)
    r = int(options.range)

    ofile = open("weights.txt", "w")
    ofile.write("%d\n" % n)
    for i in range(n):
        ofile.write("%d\n" % random.randint(1, r))
    ofile.close()


# python weight_gen.py -n 181 -r 16