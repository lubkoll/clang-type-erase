import argparse
import os


def create_parser():
    parser = argparse.ArgumentParser(description='Generates tests cases')
    parser.add_argument('--namespace-name', nargs='?', type=str, required=False,
                        default='',
                        help='namespace containing the interface')
    parser.add_argument('--class-name', nargs='?', type=str, required=True,
                        default='',
                        help='name of the interface')
    parser.add_argument('--form', nargs='?', type=str, required=False,
                    default='interface_test.form',
                    help='form containing templates for the test cases')
    parser.add_argument('--test-file', nargs='?', type=str, required=False,
                    default='interface_test.cpp',
                    help='cpp-file containing the generated unit tests')
    parser.add_argument('--sbo', action='store_true',
                        help='generate tests for small and large objects')
    return parser


if __name__ == "__main__":
    parser = create_parser()
    args = parser.parse_args()
    if args.sbo:
        args.form = args.form.replace(os.path.basename(args.form), 'sbo_' + os.path.basename(args.form))
    form = open(args.form,'r')
    test_file = open(args.test_file,'w')

    for line in form:
        line = line.replace('%class_name%', args.class_name)
        line = line.replace('%namespace_name%', args.namespace_name)
        test_file.write(line)

    form.close()
    test_file.close()
