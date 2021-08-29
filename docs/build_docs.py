# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

from pathlib import Path
import scippbuildtools as sbt

if __name__ == '__main__':
    args, _ = sbt.docs_argument_parser().parse_known_args()
    docs_dir = str(Path(__file__).parent.absolute())
    builder = sbt.DocsBuilder(docs_dir=docs_dir, **vars(args))
    builder.run_sphinx(builder=args.builder, docs_dir=docs_dir)
