import re
from typing import List, Optional

PLOT_PATTERN = re.compile(r'\.plot\(')
PLOT_DIRECTIVE_PATTERN = re.compile(r'^\s*\.\. plot::.*')


def previous_nonempty_line(lines: List[str], index: int) -> Optional[str]:
    index -= 1
    while index >= 0 and not lines[index].strip():
        index -= 1
    return lines[index] if index >= 0 else None


def indentation_of(s: str) -> int:
    return len(s) - len(s.lstrip())


def process_block(lines: List[str], begin: int, end: int) -> List[str]:
    block = lines[begin:end]
    if PLOT_PATTERN.search(block[-1]) is not None:
        prev = previous_nonempty_line(lines, begin)
        if prev and PLOT_DIRECTIVE_PATTERN.match(prev) is None:
            # If the block is not preceded by any text or directives (e.g. comes right
            # after the 'Examples' header), its indentation is stripped in `lines`.
            # Adding some indentation here fixes that but also modifies indentation
            # if the block is already indented. That should not cause problems.
            return [
                ' ' * indentation_of(prev) + '.. plot::', '',
                *map(lambda l: '    ' + l, block)
            ]
    return block


def is_start_of_block(line: str) -> bool:
    return line.lstrip().startswith('>>>')


def is_part_of_block(line: str) -> bool:
    stripped = line.lstrip()
    return stripped.startswith('>>>') or stripped.startswith('...')


def add_plot_directives(app, what, name, obj, options, lines: List[str]):
    new_lines = []
    block_begin = None
    for i, line in enumerate(lines):
        if block_begin is None:
            if is_start_of_block(line):
                # Begin a new block.
                block_begin = i
            else:
                # Not in a block -> copy line into output.
                new_lines.append(line)
        else:
            if not is_part_of_block(line):
                # Block has ended.
                new_lines.extend(process_block(lines, block_begin, i))
                new_lines.append(line)
                block_begin = None
            # else: Continue block.

    lines[:] = new_lines


def setup(app):
    app.connect('autodoc-process-docstring', add_plot_directives)
    return {'version': 1, 'parallel_read_safe': True}
