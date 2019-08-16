# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from IPython.core.display import display, HTML

import numpy as np
import scipp as sc

_colors = {
    'coord': ['dde9af', 'bcd35f', '89a02c'],
    'data': ['ffe680', 'ffd42a', 'd4aa00'],
    'labels': ['afdde9', '5fbcd3', '2c89a0'],
    'attr': ['ff8080', 'ff2a2a', 'd40000'],
    'inactive': ['cccccc', '888888', '444444']
}

# Unit is `em`. This particular value is chosen to avoid a horizontal scroll
# bar with the readthedocs theme.
_svg_width = 40

_cubes_in_full_width = 24

# We are effectively rescaling our svg by setting an explicit viewport size.
# Here we compute relative font sizes, based on a cube width of "1" (px).
_svg_em = _cubes_in_full_width / _svg_width
_normal_font = round(_svg_em, 2)
_small_font = round(0.8 * _svg_em, 2)
_smaller_font = round(0.6 * _svg_em, 2)


class VariableDrawer():
    def __init__(self, variable, margin=1.0, target_dims=None):
        self._margin = margin
        self._variable = variable
        self._target_dims = target_dims
        if self._target_dims is None:
            self._target_dims = self._variable.dims
        # special extent value indicating sparse dimension
        self._sparse_flag = -1
        self._sparse_box_scale = 0.3

    def _draw_box(self, origin_x, origin_y, color, xlen=1):
        return " ".join([
            '<rect',
            'style="fill:#{};fill-opacity:1;stroke:#000;stroke-width:0.05"',
            'id="rect"',
            'width="xlen" height="1" x="origin_x" y="origin_y"/>',
            '<path',
            'style="fill:#{};stroke:#000;stroke-width:0.05;stroke-linejoin:round"',  # noqa #501
            'd="m origin_x origin_y l 0.3 -0.3 h xlen l -0.3 0.3 z"',
            'id="path1" />',
            '<path',
            'style="fill:#{};stroke:#000;stroke-width:0.05;stroke-linejoin:round"',  # noqa #501
            'd="m origin_x origin_y m xlen 0 l 0.3 -0.3 v 1 l -0.3 0.3 z"',
            'id="path2" />'
        ]).format(*color).replace("origin_x", str(origin_x)).replace(
            "origin_y", str(origin_y)).replace("xlen", str(xlen))

    def _variance_offset(self):
        shape = self._extents()
        depth = shape[-3] + 1
        return 0.3 * depth

    def _extents(self):
        """Compute 3D extent, remapping dimension order to target dim order"""
        shape = self._variable.shape
        dims = self._variable.dims
        d = dict(zip(dims, shape))
        e = []
        for dim in self._target_dims:
            if dim in d:
                e.append(d[dim])
            elif dim in dims:
                e.append(self._sparse_flag)
            else:
                e.append(1)
        return [1] * (3 - len(e)) + e

    def _sparse_extent(self):
        extent = 0
        if isinstance(self._variable, sc.DataConstProxy):
            # Sparse items in a dataset should always have a coord,
            # but may have not data
            coord = self._variable.coords[self._variable.dims[-1]]
            data = coord.values
        else:
            data = self._variable.values
        for vals in data:
            extent = max(extent, len(vals))
        return extent

    def size(self):
        """Return the size (width and height) of the rendered output"""
        width = 2 * self._margin
        height = 3 * self._margin  # double margin on top for title space
        shape = self._extents()

        if shape[-1] == self._sparse_flag:
            shape[-1] = self._sparse_box_scale * self._sparse_extent()
        width += shape[-1]
        height += shape[-2]
        depth = shape[-3]

        extra_item_count = 0
        if self._variable.variances is not None:
            extra_item_count += 1
        if isinstance(self._variable, sc.DataConstProxy):
            if self._variable.sparse:
                for name, label in self._variable.labels:
                    if label.sparse:
                        extra_item_count += 1
                sparse_dim = self._variable.sparse_dim
                for dim, coord in self._variable.coords:
                    if dim == sparse_dim:
                        extra_item_count += 1
        if self._variable.values is None:
            # No data
            extra_item_count -= 1
        depth += extra_item_count * (depth + 1)
        width += 0.3 * depth
        height += 0.3 * depth
        return [width, height]

    def _draw_array(self, color, data, offset=[0, 0]):
        """Draw the array of boxes"""
        shape = self._variable.shape
        dx = offset[0]
        dy = offset[1] + 0.3  # extra offset for top face of top row of cubes
        svg = ''

        if len(shape) <= 3:
            lz, ly, lx = self._extents()
            for z in range(lz):
                for y in reversed(range(ly)):
                    true_lx = lx
                    x_scale = 1
                    sparse = False
                    if lx == self._sparse_flag:
                        # TODO This works only for 2D and no transpose
                        true_lx = len(data[ly - y - 1])
                        if true_lx == 0:
                            true_lx = 1
                            x_scale *= 0
                        x_scale *= self._sparse_box_scale
                        sparse = True
                    for x in range(true_lx):
                        # Do not draw hidden boxes
                        if not sparse:
                            if z != lz - 1 and y != 0 and x != lx - 1:
                                continue
                        svg += self._draw_box(
                            dx + x * x_scale + self._margin + 0.3 *
                            (lz - z - 1), dy + y + 2 * self._margin + 0.3 * z,
                            color, x_scale)
        return svg

    def _draw_labels(self, offset):
        dims = self._variable.dims
        shape = self._variable.shape
        view_height = self.size()[1]
        svg = ''
        dx = offset[0]
        dy = offset[1]

        def make_label(dim, extent, axis):
            if axis == 2:
                return '<text x="{}" y="{}" text-anchor="middle" fill="dim-color" \
                        style="font-size:#smaller-font">{}</text>'.format(
                    dx + self._margin + 0.5 * extent,
                    dy + view_height - self._margin + _smaller_font, dim)
            if axis == 1:
                return '<text x="x_pos" y="y_pos" text-anchor="middle" \
                    fill="dim-color" style="font-size:#smaller-font" \
                    transform="rotate(-90, x_pos, y_pos)">{}</text>'.replace(
                    'x_pos',
                    str(dx + self._margin - 0.3 * _smaller_font)).replace(
                        'y_pos',
                        str(dy + view_height - self._margin -
                            0.5 * extent)).format(dim)
            if axis == 0:
                return '<text x="x_pos" y="y_pos" text-anchor="middle" \
                    fill="dim-color" style="font-size:#smaller-font" \
                    transform="rotate(-45, x_pos, y_pos)">{}</text>'.replace(
                    'x_pos',
                    str(dx + self._margin + 0.3 * 0.5 * extent -
                        0.2 * _smaller_font)).replace(
                            'y_pos',
                            str(dy + view_height - self._margin -
                                self._extents()[-2] - 0.3 * 0.5 * extent -
                                0.2 * _smaller_font)).format(dim)

        for dim, extent in zip(dims, shape):
            svg += make_label(
                dim, extent,
                self._target_dims.index(dim) + (3 - len(self._target_dims)))
        return svg

    def _draw_info(self, offset, title):
        try:
            unit = str(self._variable.unit)
        except Exception:
            unit = '(undefined)'
        details = 'dims={}, shape={}, unit={}, variances={}'.format(
            self._variable.dims, self._variable.shape, unit,
            self._variable.variances is not None)
        if title is not None:
            svg = '<text x="{}" y="{}" \
                    style="font-size:#normal-font">{}</text>'.format(
                offset[0] + 0, offset[1] + 0.6, title)
            svg += '<title>{}</title>'.format(details)
        else:
            svg = '<text x="{}" y="{}" \
                    style="font-size:#small-font">\
                    {}\
                    </text>'.format(offset[0] + 0, offset[1] + 0.6, details)
        return svg

    def draw(self, color, offset=np.zeros(2), title=None):
        svg = '<g>'
        svg += self._draw_info(offset, title)
        items = []
        if self._variable.variances is not None:
            items.append(('variances', self._variable.variances, color))
        if self._variable.values is not None:
            items.append(('values', self._variable.values, color))
        if isinstance(self._variable, sc.DataConstProxy):
            if self._variable.sparse:
                for name, label in self._variable.labels:
                    if label.sparse:
                        items.append((name, label.values, _colors['labels']))
                sparse_dim = self._variable.sparse_dim
                for dim, coord in self._variable.coords:
                    if dim == sparse_dim:
                        items.append((str(sparse_dim),
                                      self._variable.coords[sparse_dim].values,
                                      _colors['coord']))

        for i, (name, data, color) in enumerate(items):
            svg += '<g>'
            svg += '<title>{}</title>'.format(name)
            svg += self._draw_array(
                color=color,
                offset=offset +
                np.array([(len(items) - i - 1) * self._variance_offset(),
                          i * self._variance_offset()]),
                data=data)
            svg += '</g>'
            svg += self._draw_labels(offset=offset)
        svg += '</g>'
        return svg.replace('#normal-font',
                           '{}px'.format(_normal_font)).replace(
                               '#small-font',
                               '{}px'.format(_small_font)).replace(
                                   '#smaller-font',
                                   '{}px'.format(_smaller_font))

    def _set_colors(self, svg):
        dim_color = '#444444'
        return svg.replace('dim-color', dim_color)

    def make_svg(self):
        return self._set_colors(
            '<svg width={}em viewBox="0 0 {} {}">{}</svg>'.format(
                _svg_width, max(_cubes_in_full_width,
                                self.size()[0]),
                self.size()[1], self.draw(color=_colors['data'])))


class DatasetDrawer():
    def __init__(self, dataset):
        self._dataset = dataset

    def _dims(self):
        # The dimension-order in a dataset is not defined. However, here we
        # need one for the practical purpose of drawing variables with
        # consistent ordering. We simply use that of the item with highest
        # dimension count.
        count = -1
        if isinstance(self._dataset, sc.DataConstProxy):
            dims = self._dataset.dims
            count = len(dims)
        else:
            for name, item in self._dataset:
                if len(item.dims) > count:
                    dims = item.dims
                    count = len(dims)
        return dims

    def make_svg(self, dataset):
        content = ''
        width = 0
        height = 0
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
        area_x = []
        area_y = []
        area_z = []
        area_xy = []
        area_0d = []
        if isinstance(self._dataset, sc.DataConstProxy):
            area_xy.append(('', self._dataset, _colors['data']))
        else:
            # Render highest-dimension items last so coords are optically
            # aligned
            for name, data in dataset:
                item = (name, data, _colors['data'])
                if data.dims != dims:
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

        for dim, coord in dataset.coords:
            if coord.sparse:
                continue
            item = (dim, coord, _colors['coord'])
            if dim == dims[-1]:
                area_x.append(item)
            elif dim == dims[-2]:
                area_y.append(item)
            else:
                area_z.append(item)

        for name, labels in dataset.labels:
            if labels.sparse:
                continue
            dim = labels.dims[-1]
            item = (name, labels, _colors['labels'])
            if dim == dims[-1]:
                area_x.append(item)
            elif dim == dims[-2]:
                area_y.append(item)
            else:
                area_z.append(item)

        for name, attr in dataset.attrs:
            if attr.sparse:
                continue
            dim = attr.dims[-1]
            item = (name, attr, _colors['attr'])
            if dim == dims[-1]:
                area_x.append(item)
            elif dim == dims[-2]:
                area_y.append(item)
            else:
                area_z.append(item)

        def draw_area(area, layout_direction, reverse=False):
            content = ''
            width = 0
            height = 0
            offset = [0, 0]
            if reverse:
                area = reversed(area)
            for name, data, color in area:
                drawer = VariableDrawer(data, margin, target_dims=dims)
                content += drawer.draw(color=color, offset=offset, title=name)
                size = drawer.size()
                if layout_direction == 'x':
                    width += size[0]
                    height = max(height, size[1])
                    offset = [width, 0]
                else:
                    width = max(width, size[0])
                    height += size[1]
                    offset = [0, height]
            return content, width, height

        top = 0
        left = 0

        c, w, h = draw_area(area_xy, 'y')
        content += '<g transform="translate(0,{})">{}</g>'.format(height, c)
        height += h
        width += w

        c, w, h = draw_area(area_z, 'x')
        content += '<g transform="translate({},{})">{}</g>'.format(
            width, height - h, c)
        width += w

        c, w_y, h = draw_area(area_y, 'x', reverse=True)
        content += '<g transform="translate({},{})">{}</g>'.format(
            -w_y, height - h, c)

        c, w_0d, h = draw_area(area_0d, 'x', reverse=True)
        content += '<g transform="translate({},{})">{}</g>'.format(
            -w_0d, height, c)
        width += max(w_y, w_0d)
        left -= max(w_y, w_0d)

        c, w, h = draw_area(area_x, 'y')
        content += '<g transform="translate(0,{})">{}</g>'.format(height, c)
        height += h

        return '<svg width={}em viewBox="{} {} {} {}">{}</svg>'.format(
            _svg_width, left, top, max(_cubes_in_full_width, width), height,
            content)


def show(container):
    """
    Produce a graphical representation of a variable or dataset.
    """
    if isinstance(container, sc.Variable) or isinstance(
            container, sc.VariableProxy):
        draw = VariableDrawer(container)
        display(HTML(draw.make_svg()))
    else:
        draw = DatasetDrawer(container)
        display(HTML(draw.make_svg(container)))
