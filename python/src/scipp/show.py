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


class VariableDrawer():
    def __init__(self, variable, margin=1.0, target_dims=None):
        self._margin = margin
        self._variable = variable
        self._target_dims = target_dims
        if self._target_dims is None:
            self._target_dims = self._variable.dims

    def _draw_box(self, origin_x, origin_y, color):
        return """
      <rect
         style="fill:#{};fill-opacity:1;stroke:#000000;stroke-width:0.05;stroke-linejoin:round;stroke-miterlimit:4;stroke-opacity:1"
         id="rect"
         width="1" height="1" x="origin_x" y="origin_y" />
      <path
         style="fill:#{};fill-opacity:1;fill-rule:evenodd;stroke:#000000;stroke-width:0.05;stroke-linecap:butt;stroke-linejoin:round;stroke-miterlimit:4;stroke-opacity:1"
         d="m origin_x origin_y l 0.3 -0.3 h 1 l -0.3 0.3 z"
         id="path1" />
      <path
         style="fill:#{};fill-opacity:1;fill-rule:evenodd;stroke:#000000;stroke-width:0.05;stroke-linecap:butt;stroke-linejoin:round;stroke-miterlimit:4;stroke-opacity:1"
         d="m origin_x origin_y m 1 0 l 0.3 -0.3 v 1 l -0.3 0.3 z"
         id="path2" />
         """.format(*color).replace("origin_x", str(origin_x)).replace(
            "origin_y", str(origin_y))

    def _variance_offset(self):
        shape = self._variable.shape
        if len(shape) <= 2:
            depth = 2
        else:
            depth = shape[-3] + 1
        return 0.3 * depth

    def _extents(self):
        shape = self._variable.shape
        dims = self._variable.dims
        d = dict(zip(dims, shape))
        e = []
        for dim in self._target_dims:
            if dim in d:
                e.append(d[dim])
            else:
                e.append(1)
        return [1] * (3 - len(e)) + e

    def size(self):
        width = 2 * self._margin
        height = 2 * self._margin
        shape = self._extents()

        width += shape[-1]
        height += shape[-2]
        depth = shape[-3]

        if self._variable.has_variances:
            depth += depth + 1
        width += 0.3 * depth
        height += 0.3 * depth
        return [width, height]

    def _draw_array(self, color, offset=[0, 0]):
        shape = self._variable.shape
        dx = offset[0]
        dy = offset[1] + 0.3  # extra offset for top face of top row of cubes
        svg = ''

        if len(shape) <= 3:
            lz, ly, lx = self._extents()
            for z in range(lz):
                for y in reversed(range(ly)):
                    for x in range(lx):
                        svg += self._draw_box(
                            dx + x + self._margin + 0.3 *
                            (lz - z - self._margin),
                            dy + y + self._margin + 0.3 * z, color)
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
                        style="font-size:0.2px">{}</text>'.format(
                    dx + self._margin + 0.5 * extent,
                    dy + view_height - self._margin + 0.3, dim)
            if axis == 1:
                return '<text x="x_pos" y="y_pos" text-anchor="middle" \
                    fill="dim-color" style="font-size:0.2px" \
                    transform="rotate(-90, x_pos, y_pos)">{}</text>'.replace(
                    'x_pos', str(dx + self._margin - 0.2)).replace(
                        'y_pos',
                        str(dy + view_height - self._margin - 0.2 -
                            0.5 * extent)).format(dim)
            if axis == 0:
                return '<text x="x_pos" y="y_pos" text-anchor="middle" \
                    dominant-baseline="central" \
                    fill="dim-color" style="font-size:0.2px" \
                    transform="rotate(-45, x_pos, y_pos)">{}</text>'.replace(
                    'x_pos',
                    str(dx + self._margin + 0.3 * 0.5 * extent - 0.1)).replace(
                        'y_pos',
                        str(dy + view_height - self._margin -
                            self._extents()[-2] - 0.3 * 0.5 * extent -
                            0.1)).format(dim)

        for dim, extent in zip(dims, shape):
            svg += make_label(
                dim, extent,
                self._target_dims.index(dim) + (3 - len(self._target_dims)))
        return svg

    def _draw_info(self, offset, title):
        details = 'dims={}, shape={}, unit={}, variances={}'.format(
            self._variable.dims, self._variable.shape,
            str(self._variable.unit), self._variable.has_variances)
        if title is not None:
            svg = '<text x="{}" y="{}" \
                    style="font-size:0.4px">{}</text>'.format(
                offset[0] + 0, offset[1] + 0.6, title)
            svg += '<title>{}</title>'.format(details)
        else:
            svg = '<text x="{}" y="{}" \
                    style="font-size:0.2px">\
                    {}\
                    </text>'.format(offset[0] + 0, offset[1] + 0.6, details)
        return svg

    def draw(self, color, offset=np.zeros(2), title=None):
        svg = '<g>'
        svg += self._draw_info(offset, title)
        if self._variable.has_variances:
            svg += self._draw_array(color=color,
                                    offset=offset +
                                    np.array([self._variance_offset(), 0]))
            svg += self._draw_array(color=color,
                                    offset=offset +
                                    np.array([0, self._variance_offset()]))
            svg += self._draw_labels(offset=offset)
        else:
            svg += self._draw_array(color=color, offset=offset)
            svg += self._draw_labels(offset=offset)
        svg += '</g>'
        return svg

    def _set_colors(self, svg):
        dim_color = '#444444'
        return svg.replace('dim-color', dim_color)

    def make_svg(self):
        return self._set_colors('<svg viewBox="0 0 {} {}">{}</svg>'.format(
            max(16,
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
        if isinstance(self._dataset, sc.Dataset):
            for name, item in self._dataset:
                if len(item.dims) > count:
                    dims = item.dims
                    count = len(dims)
        else:
            dims = self._dataset.dims
            count = len(dims)
        return dims

    def make_svg(self, dataset):
        content = ''
        width = 0
        height = 0
        margin = 0.8
        dims = self._dims()
        # TODO bin edges (offset by 0.5)
        # TODO font scaling and other scaling issues
        # TODO sparse variables
        # TODO limit number of drawn cubes if shape exceeds certain limit
        #      (draw just a single cube with correct edge proportions?)
        if isinstance(self._dataset, sc.Dataset):
            items = []
            for name, data in dataset:
                if data.dims != dims:
                    items.append((name, data))
            # Render highest-dimension items last so coords are optically aligned
            for name, data in dataset:
                if data.dims == dims:
                    items.append((name, data))

            for name, data in items:
                drawer = VariableDrawer(data, margin, target_dims=dims)
                content += drawer.draw(color=_colors['data'],
                                       offset=[0, height],
                                       title=name)
                size = drawer.size()
                width = max(width, size[0])
                coord_1_y = height
                height += size[1]
        else:
            drawer = VariableDrawer(self._dataset, margin, target_dims=dims)
            content += drawer.draw(color=_colors['data'],
                                   offset=[0, height],
                                   title='')
            size = drawer.size()
            width = max(width, size[0])
            coord_1_y = height
            height += size[1]

        coord_2_x = width
        coord_2_y = height

        # It might be better to draw coords on the top and left instead of
        # bottom and right, but this way is easier for offset computation.
        # Maybe just use an svg offset transform to accomplish this in a
        # nice way?
        for dim, coord in dataset.coords:
            drawer = VariableDrawer(coord, margin, target_dims=dims)
            size = drawer.size()
            if dim == dims[-1]:
                content += drawer.draw(color=_colors['coord'],
                                       offset=[0, height],
                                       title=dim)
                width = max(width, size[0])
                height += size[1]
            elif dim == dims[-2]:
                content += drawer.draw(color=_colors['coord'],
                                       offset=[width, coord_1_y],
                                       title=dim)
                width += size[0]
            else:
                content += drawer.draw(color=_colors['coord'],
                                       offset=[coord_2_x, coord_2_y],
                                       title=dim)
                coord_2_x += size[0]

        for name, labels in dataset.labels:
            drawer = VariableDrawer(labels, margin, target_dims=dims)
            size = drawer.size()
            if labels.dims[-1] == dims[-1]:
                content += drawer.draw(color=_colors['labels'],
                                       offset=[0, height],
                                       title=name)
                width = max(width, size[0])
                height += size[1]
            elif labels.dims[-1] == dims[-2]:
                content += drawer.draw(color=_colors['labels'],
                                       offset=[width, coord_1_y],
                                       title=name)
                width += size[0]
            else:
                content += drawer.draw(color=_colors['labels'],
                                       offset=[coord_2_x, coord_2_y],
                                       title=name)
                coord_2_x += size[0]

        for name, attr in dataset.attrs:
            drawer = VariableDrawer(attr, margin, target_dims=dims)
            content += drawer.draw(color=_colors['attr'],
                                   offset=[0, height],
                                   title=name)
            size = drawer.size()
            width = max(width, size[0])
            height += size[1]

        return '<svg viewBox="0 0 {} {}">{}</svg>'.format(
            max(16, width), height, content)


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
