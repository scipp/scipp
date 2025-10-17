# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

from html import escape
from typing import Any

import numpy as np

from ..core import DataArray, Dataset, Variable
from . import colors
from .resources import load_style

# Unit is `em`. This particular value is chosen to avoid a horizontal scroll
# bar with the readthedocs theme.
_svg_width = 40

_cubes_in_full_width = 24

# We are effectively rescaling our svg by setting an explicit viewport size.
# Here we compute relative font sizes, based on a cube width of "1" (px).
_svg_em = _cubes_in_full_width / _svg_width
_large_font = round(1.6 * _svg_em, 2)
_normal_font = round(_svg_em, 2)
_small_font = round(0.8 * _svg_em, 2)
_smaller_font = round(0.6 * _svg_em, 2)


def _color_variants(hex_color: str) -> tuple[str, str, str]:
    """
    Produce darker and lighter color variants, given an input color.
    """
    rgb = colors.hex_to_rgb(hex_color)
    dark = colors.rgb_to_hex(np.clip(rgb - 40, 0, 255))
    light = colors.rgb_to_hex(np.clip(rgb + 30, 0, 255))
    return light, hex_color, dark


def _truncate_long_string(long_string: str, max_title_length: int) -> str:
    return (
        (long_string[:max_title_length] + '..')
        if len(long_string) > max_title_length
        else long_string
    )


def _build_svg(
    content: Any, left: float, top: float, width: float, height: float
) -> str:
    return (
        f'<svg width={_svg_width}em viewBox="{left} {top} {width} {height}"'
        ' class="sc-root">'
        f'<defs>{load_style()}</defs>{content}</svg>'
    )


class VariableDrawer:
    def __init__(
        self,
        variable: Variable,
        margin: float = 1.0,
        target_dims: tuple[str, ...] | None = None,
        show_alignment: bool = False,
    ) -> None:
        self._margin = margin
        self._variable = variable
        self._target_dims = target_dims or self._dims()
        self._show_alignment = show_alignment
        self._x_stride = 1
        if len(self._dims()) > 3:
            raise RuntimeError(f"Cannot visualize {len(self._dims())}-D data")

    def _dims(self) -> tuple[str, ...]:
        return self._variable.dims

    def _draw_box(
        self, origin_x: float, origin_y: float, color: str, xlen: int = 1
    ) -> str:
        return (
            " ".join(
                [
                    '<rect',
                    'style="fill:{};fill-opacity:1;stroke:#000;stroke-width:0.05"',
                    'id="rect"',
                    'width="xlen" height="1" x="origin_x" y="origin_y"/>',
                    '<path',
                    'style="fill:{};stroke:#000;stroke-width:0.05;stroke-linejoin:round"',
                    'd="m origin_x origin_y l 0.3 -0.3 h xlen l -0.3 0.3 z"',
                    'id="path1" />',
                    '<path',
                    'style="fill:{};stroke:#000;stroke-width:0.05;stroke-linejoin:round"',
                    'd="m origin_x origin_y m xlen 0 l 0.3 -0.3 v 1 l -0.3 0.3 z"',
                    'id="path2" />',
                ]
            )
            .format(*_color_variants(color))
            .replace("origin_x", str(origin_x))
            .replace("origin_y", str(origin_y))
            .replace("xlen", str(xlen))
        )

    def _draw_dots(self, x0: float, y0: float) -> str:
        dots = ""
        for x, y in 0.1 + 0.8 * np.random.rand(7, 2) + np.array([x0, y0]):
            dots += f'<circle cx="{x}" cy="{y}" r="0.07"/>'
        return dots

    def _variance_offset(self) -> float:
        shape = self._extents()
        depth = shape[-3] + 1
        return 0.3 * depth

    def _extents(self) -> list[int]:
        """Compute 3D extent, remapping dimension order to target dim order"""
        shape = self._variable.shape
        dims = self._dims()
        d = dict(zip(dims, shape, strict=True))
        e = []
        max_extent = _cubes_in_full_width // 2
        for dim in self._target_dims:
            if dim in d:
                e.append(min(d[dim], max_extent))
            else:
                e.append(1)
        return [1] * (3 - len(e)) + e

    def _events_height(self) -> float:
        if not self._variable.is_binned:
            return 0
        events = self._variable.bins.constituents['data']
        # Rough estimate of vertical space taken by depiction of events buffer
        if isinstance(events, Variable):
            return 1
        elif isinstance(events, Dataset):
            raise ValueError("Cannot visualize Dataset events")
        else:
            return 1 + 1.3 * (len(events.coords) + len(events.masks))

    def size(self) -> tuple[float, float]:
        """Return the size (width and height) of the rendered output"""
        width = 2 * self._margin
        height = 3 * self._margin  # double margin on top for title space
        shape = self._extents()

        width += shape[-1]
        height += shape[-2]
        depth = shape[-3]

        extra_item_count = 0
        if self._variable.variances is not None:
            extra_item_count += 1
        if self._variable.values is None:
            # No data
            extra_item_count -= 1
        depth += extra_item_count * (depth + 1)
        width += 0.3 * depth
        height += 0.3 * depth
        height = max(height, self._events_height())
        return width, height

    def _draw_array(
        self,
        color: str,
        offset: tuple[float, float] | None = None,
        events: bool = False,
    ) -> str:
        """Draw the array of boxes"""
        offset = offset or (0, 0)
        dx = offset[0]
        dy = offset[1] + 0.3  # extra offset for top face of top row of cubes
        svg = ''

        lz, ly, lx = self._extents()
        for z in range(lz):
            for y in reversed(range(ly)):
                true_lx = lx
                x_scale = 1
                for x in range(true_lx):
                    # Do not draw hidden boxes
                    if z != lz - 1 and y != 0 and x != lx - 1:
                        continue
                    origin_x = dx + x * x_scale + self._margin + 0.3 * (lz - z - 1)
                    origin_y = dy + y + 2 * self._margin + 0.3 * z
                    svg += self._draw_box(origin_x, origin_y, color, x_scale)
                    if events:
                        svg += self._draw_dots(origin_x, origin_y)
        return svg

    def _draw_labels(self, offset: tuple[float, float]) -> str:
        view_height = self.size()[1]
        svg = ''
        dx = offset[0]
        dy = offset[1]

        def make_label(dim: str, extent: float, axis: int) -> str:
            if axis == 2:
                x_pos = dx + self._margin + 0.5 * extent
                y_pos = dy + view_height - self._margin + _smaller_font
                return f'<text x="{x_pos}" y="{y_pos}"\
                         class="sc-label" \
                         style="font-size:#smaller-font">{escape(dim)}</text>'

            if axis == 1:
                x_pos = dx + self._margin - 0.3 * _smaller_font
                y_pos = dy + view_height - self._margin - 0.5 * extent
                return f'<text x="{x_pos}" y="{y_pos}" \
                    class="sc-label" style="font-size:#smaller-font" \
                    transform="rotate(-90, {x_pos}, {y_pos})">\
                        {escape(dim)}</text>'

            if axis == 0:
                x_pos = dx + self._margin + 0.3 * 0.5 * extent - 0.2 * _smaller_font
                y_pos = (
                    dy
                    + view_height
                    - self._margin
                    - self._extents()[-2]
                    - 0.3 * 0.5 * extent
                    - 0.2 * _smaller_font
                )
                return f'<text x="{x_pos}" y="{y_pos}" \
                    class="sc-label" style="font-size:#smaller-font" \
                    transform="rotate(-45, {x_pos}, {y_pos})">\
                        {escape(dim)}</text>'

            raise ValueError(f"Invalid axis: {axis}")

        extents = self._extents()
        for dim in self._variable.dims:
            i = self._target_dims.index(dim) + (3 - len(self._target_dims))
            # 1 is a dummy extent so events dim label is drawn at correct pos
            extent = max(extents[i], 1)
            svg += make_label(dim, extent, i)
        return svg

    def _draw_info(self, offset: tuple[float, float], title: str | None) -> str:
        try:
            unit = str(self._variable.unit)
        except Exception:
            unit = '(undefined)'
        details = 'dims={}, shape={}, unit={}, variances={}'.format(
            self._variable.dims,
            self._variable.shape,
            unit,
            self._variable.variances is not None,
        )
        x_pos = offset[0]
        y_pos = offset[1] + 0.6
        weight = (
            "font-weight: bold"
            if self._show_alignment and self._variable.aligned
            else ""
        )
        if title is not None:
            # Crudely avoid label overlap, ignoring actual character width
            brief_title = _truncate_long_string(str(title), int(2.5 * self.size()[0]))
            svg = f'<text x="{x_pos}" y="{y_pos}" \
                    class="sc-name" style="font-size:#normal-font;{weight}"> \
                    {escape(brief_title)}</text>'

            svg += f'<title>{title}({escape(details)})</title>'
        else:
            svg = f'<text x="{x_pos}" y="{y_pos}" \
                    class="sc-name" style="font-size:#small-font;{weight}"> \
                    {escape(details)}</text>'

        return svg

    def _draw_bins_buffer(self) -> str:
        if not self._variable.is_binned:
            return ''
        svg = ''
        x0 = self._margin + self._extents()[-1]
        y0 = 2 * self._margin + 0.3 * self._extents()[-3]
        height = self._events_height()
        style = 'class="sc-inset-line"'
        svg += f'<line x1={x0} y1={y0 + 0} x2={x0 + 2} y2={y0 - 1} {style}/>'
        svg += f'<line x1={x0} y1={y0 + 1} x2={x0 + 2} y2={y0 + height} {style}/>'
        svg += '<g transform="translate({},{}) scale(0.5)">{}</g>'.format(
            self.size()[0] + 1,
            0,
            make_svg(self._variable.bins.constituents['data'], content_only=True),
        )
        return svg

    def draw(
        self,
        color: str,
        offset: tuple[float, float] | None = None,
        title: str | None = None,
    ) -> str:
        offset = (0, 0) if offset is None else offset
        svg = '<g>'
        svg += self._draw_info(offset, title)
        items = []
        if self._variable.variances is not None:
            items.append(('variances', color))
        if self._variable.values is not None:
            items.append(('values', color))

        for i, (name, color) in enumerate(items):
            svg += '<g>'
            svg += f'<title>{name}</title>'
            svg += self._draw_array(
                color=color,
                offset=(
                    offset[0] + (len(items) - i - 1) * self._variance_offset(),
                    offset[1] + i * self._variance_offset(),
                ),
                events=self._variable.is_binned,
            )
            svg += '</g>'
            svg += self._draw_labels(offset=offset)
        svg += '</g>'
        svg += self._draw_bins_buffer()
        return (
            svg.replace('#normal-font', f'{_normal_font}px')
            .replace('#small-font', f'{_small_font}px')
            .replace('#smaller-font', f'{_smaller_font}px')
        )

    def make_svg(self, content_only: bool = False) -> str:
        if content_only:
            return self.draw(color=colors.STYLE['data'])
        return _build_svg(
            self.make_svg(content_only=True),
            0,
            0,
            max(_cubes_in_full_width, self.size()[0]),
            self.size()[1],
        )


class DrawerItem:
    def __init__(
        self, name: str, data: Variable, color: str, show_alignment: bool = False
    ) -> None:
        self._name = name
        self._data = data
        self._color = color
        self._show_alignment = show_alignment

    def append_to_svg(
        self,
        content: str,
        width: float,
        height: float,
        offset: tuple[float, float],
        layout_direction: str,
        margin: float,
        dims: tuple[str, ...],
    ) -> tuple[str, float, float, tuple[float, float]]:
        drawer = VariableDrawer(
            self._data, margin, target_dims=dims, show_alignment=self._show_alignment
        )
        content += drawer.draw(color=self._color, offset=offset, title=self._name)
        size = drawer.size()
        width, height, offset = _new_size_and_offset(
            size, width, height, layout_direction
        )
        return content, width, height, offset


class EllipsisItem:
    @staticmethod
    def append_to_svg(
        content: str,
        width: float,
        height: float,
        offset: tuple[float, float],
        layout_direction: str,
        *unused: object,
    ) -> tuple[str, float, float, tuple[float, float]]:
        x_pos = offset[0] + 0.3
        y_pos = offset[1] + 2.0
        content += f'<text x="{x_pos}" y="{y_pos}" class="sc-label" \
                    style="font-size:{_large_font}px"> ... </text>'

        ellipsis_size = (1.5, 2.0)
        width, height, offset = _new_size_and_offset(
            ellipsis_size, width, height, layout_direction
        )
        return content, width, height, offset


def _new_size_and_offset(
    added_size: tuple[float, float], width: float, height: float, layout_direction: str
) -> tuple[float, float, tuple[float, float]]:
    if layout_direction == 'x':
        width += added_size[0]
        height = max(height, added_size[1])
        offset = (width, 0.0)
    else:
        width = max(width, added_size[0])
        height += added_size[1]
        offset = (0.0, height)
    return width, height, offset


class DatasetDrawer:
    def __init__(self, dataset: DataArray | Dataset) -> None:
        self._dataset = dataset

    def _dims(self) -> tuple[str, ...]:
        dims = self._dataset.dims
        if isinstance(self._dataset, DataArray):
            # Handle, e.g., bin edges of a slice, where data lacks the edge dim
            for item in self._dataset.coords.values():
                for dim in item.dims:
                    if dim not in dims:
                        dims = (dim, *dims)
        if len(dims) > 3:
            raise RuntimeError(f"Cannot visualize {len(dims)}-D data")
        return dims

    def make_svg(self, content_only: bool = False) -> str:
        content = ''
        width = 0.0
        height = 0.0
        margin = 0.5
        dims = self._dims()
        # TODO bin edges (offset by 0.5)
        # TODO limit number of drawn cubes if shape exceeds certain limit
        #      (draw just a single cube with correct edge proportions?)

        # We are drawing in several areas:
        #
        # (y-coords) | (data > 1d) | (z-coords)
        # -------------------------------------
        # (0d)       | (x-coords)  |
        #
        # X and Y coords are thus aligning optically with the data, and are
        # where normal axes are expected. Data that depends only on X or only
        # on Y is also drawn in the respective areas, this makes it clear
        # that the respective other coords do not apply: It avoids
        # intersection with imaginary grid lines drawn from the X coord up or
        # from the Y coord towards the right.
        # For the same reason, 0d variables are drawn in the bottom left, not
        # intersecting any of the imaginary grid lines.
        # If there is more than one data item in the center area they are
        # stacked. Unfortunately this breaks the optical alignment with the Y
        # coordinates, but in a static picture there is probably no other way.
        area_x: list[DrawerItem | EllipsisItem] = []
        area_y: list[DrawerItem | EllipsisItem] = []
        area_z: list[DrawerItem | EllipsisItem] = []
        area_xy: list[DrawerItem | EllipsisItem] = []
        area_0d: list[DrawerItem | EllipsisItem] = []
        if isinstance(self._dataset, DataArray):
            area_xy.append(DrawerItem('', self._dataset.data, colors.STYLE['data']))
        else:
            # Render highest-dimension items last so coords are optically
            # aligned
            for name, data in sorted(self._dataset.items()):
                item = DrawerItem(name, data.data, colors.STYLE['data'])
                # Using only x and 0d areas for 1-D dataset
                if len(dims) == 1 or data.dims != dims:
                    if len(data.dims) == 0:
                        area_0d.append(item)
                    elif len(data.dims) != 1:
                        area_xy[-1:-1] = [item]
                    elif data.dims[0] == dims[-1]:
                        area_x.append(item)
                    elif data.dims[0] == dims[-2]:
                        area_y.append(item)
                    else:
                        area_z.append(item)
                else:
                    area_xy.append(item)

        ds = self._dataset
        if isinstance(ds, DataArray):
            categories = zip(
                ['coords', 'masks'],
                [ds.coords, ds.masks],
                strict=True,
            )
        else:
            categories = zip(['coords'], [ds.coords], strict=True)
        for what, items in categories:
            for name, var in sorted(items.items()):
                item = DrawerItem(
                    name,
                    var,
                    colors.STYLE[what],
                    show_alignment=what == 'coords',
                )
                if len(var.dims) == 0:
                    area_0d.append(item)
                elif var.dims[-1] == dims[-1]:
                    area_x.append(item)
                elif var.dims[-1] == dims[-2]:
                    area_y.append(item)
                else:
                    area_z.append(item)

        def draw_area(
            area: list[DrawerItem | EllipsisItem],
            layout_direction: str,
            reverse: bool = False,
            truncate: bool = False,
        ) -> tuple[str, float, float]:
            number_of_items = len(area)
            min_items_before_worth_truncate = 5
            if truncate and number_of_items > min_items_before_worth_truncate:
                area[1:-1] = [EllipsisItem()]

            content = ''
            width = 0.0
            height = 0.0
            offset = (0.0, 0.0)
            for item in reversed(area) if reverse else area:
                content, width, height, offset = item.append_to_svg(
                    content, width, height, offset, layout_direction, margin, dims
                )
            return content, width, height

        top = 0.0
        left = 0.0

        c, w, h = draw_area(area_xy, 'y')
        content += f'<g transform="translate(0,{height})">{c}</g>'
        c_x, w_x, h_x = draw_area(area_x, 'y')
        c_y, w_y, h_y = draw_area(area_y, 'x', reverse=True)
        height += max(h, h_y)
        width += max(w, w_x)

        c, w, h = draw_area(area_z, 'x')
        content += f'<g transform="translate({width},{height - h})">{c}</g>'
        width += w

        content += f'<g transform="translate({-w_y},{height - h_y})">{c_y}</g>'

        c, w_0d, h_0d = draw_area(area_0d, 'x', reverse=True, truncate=True)
        content += f'<g transform="translate({-w_0d},{height})">{c}</g>'
        width += max(w_y, w_0d)
        left -= max(w_y, w_0d)

        content += f'<g transform="translate(0,{height})">{c_x}</g>'
        height += max(h_x, h_0d)

        if content_only:
            return content
        return _build_svg(content, left, top, max(_cubes_in_full_width, width), height)


def make_svg(
    container: Variable | DataArray | Dataset, content_only: bool = False
) -> str:
    """
    Return an SVG representation of a variable, data array, or dataset.
    """
    if isinstance(container, Variable):
        draw: VariableDrawer | DatasetDrawer = VariableDrawer(container)
    else:
        draw = DatasetDrawer(container)
    return draw.make_svg(content_only=content_only)


def show(container: Variable | DataArray | Dataset) -> None:
    """
    Show a graphical representation of a variable, data array, or dataset.

    See 'SVG representation' in
    `Representations and Tables <../../user-guide/representations-and-tables.rst>`_
    for details.
    """
    from IPython.display import HTML, display

    display(HTML(make_svg(container)))  # type: ignore[no-untyped-call]
