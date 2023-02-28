from argparse import ArgumentParser

from . import generate_stub
from .config import DEFAULT_TARGET


def parse_args():
    parser = ArgumentParser(
        'stubgen', description='Generate stub file for scipp classes defined in C++'
    )
    parser.add_argument(
        '--output', default=DEFAULT_TARGET, help='Place the generated file here'
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    stub = generate_stub()
    with open(args.output, 'w') as f:
        f.write(stub)


if __name__ == '__main__':
    main()
