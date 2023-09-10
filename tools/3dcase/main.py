#!/usr/bin/env python
from operator import length_hint
from solid import *
from dataclasses import dataclass
from solid.utils import *
import dataclasses
import math


class ConstantOpenSCADObject(OpenSCADObject):
    def __init__(self, code):
        super().__init__("not reander able", {})
        self.code = code

    def _render(self, render_holes=42):
        return self.code


def scad_inline(code):
    return ConstantOpenSCADObject(code)


@dataclass
class Measures:
    # Board
    board_width = 91.5
    board_depth = 58
    board_radius = 3
    board_height = 9

    # screws
    screw_diameter = 3.4
    screw_head_diameter = 7
    nut_diameter = 6.5
    nut_height = 0
    smaller_attachment_width = 20
    attachment_height = 6
    attachment_depth = 6
    # screw_outer_distance_1 = 3.35
    # screw_outer_distance_2 = 4.25
    screw_x_distance = 3.57  # 3.47
    screw_y_distance = 3.49

    # USB
    usb_depth = 11
    usb_height = 6.5
    usb_radius = 3
    usb_y_shift = 11
    usb_z_shift = 7

    # Meta
    thickness = 1.5

    # Adhesive tape
    adhesive_tape_height = 1
    adhesive_tape_width = 12.2
    adhesive_tape_spacing =  8

    # Breadboard
    breadboard_width = 47
    breadboard_depth = 35.5
    breadboard_height = 10
    breadboard_notch = 4.5
    back_port_diameter = 10
    back_port_z = 15

    # Breathing grate
    grate_width = 2
    grate_height = 8


def rounded_cube(dimensions, radius):
    (width, depth, height) = dimensions
    w_distance = (width - radius*2)/2
    d_distance = (depth - radius*2)/2

    length = hull()(
        translate((-w_distance, 0, 0)
                  )(cylinder(r=radius, h=height, center=True)),
        translate((+w_distance, 0, 0)
                  )(cylinder(r=radius, h=height, center=True)),
    )

    return hull()(
        translate((0, -d_distance, 0))(length),
        translate((0, +d_distance, 0))(length)
    )


def build_grate(width, depth, height, span, center=False):
    number = int(span / (width*2))

    grate = hull()(
        translate((0, -height/2+width/2, 0)
                  )(cylinder(d=width, h=depth, center=center)),
        translate((0, height/2-width/2, 0)
                  )(cylinder(d=width, h=depth, center=center)),
    )
    result = union()()

    for i in range(number):
        result += translate((i*width*2, 0, 0))(grate)

    shift = -width * (number - 1)

    return translate((shift, 0, 0))(result)


def piece(m: Measures):
    rotation_angle = 60
    case_height = m.board_height+m.attachment_height

    # Raw WTSC01 case
    raw_figure = rounded_cube(
        (m.board_width+m.thickness*2, m.board_depth+m.thickness*2, case_height), m.board_radius)
    internal_raw_figure = rounded_cube(
        (m.board_width, m.board_depth, case_height-m.thickness), m.board_radius)
    raw_screen_clearing = rounded_cube(
        (m.board_width, m.board_depth, case_height), m.board_radius)

    case = raw_figure - \
        rounded_cube((m.board_width, m.board_depth,
                     case_height), m.board_radius)

    top_screw_attachment = rounded_cube(
        (m.board_width+m.attachment_depth*2, m.attachment_depth*2, m.attachment_height), m.attachment_depth)

    bottom_screw_attachment = rounded_cube(
        (m.smaller_attachment_width + m.attachment_depth, m.attachment_depth*2, m.attachment_height), m.attachment_depth)

    case += translate((0, m.board_depth/2, m.board_height/2)
                      )(top_screw_attachment)

    case += translate((-(m.board_width-m.smaller_attachment_width) /
                      2-m.attachment_depth/2, -m.board_depth/2, m.board_height/2))(bottom_screw_attachment)
    case += translate(((m.board_width-m.smaller_attachment_width) /
                      2+m.attachment_depth/2, -m.board_depth/2, m.board_height/2))(bottom_screw_attachment)

    screw_hole = cylinder(d=m.screw_diameter,
                          h=m.attachment_height, center=True)
    nut_hole = union()(
        scad_inline("\n$fn=6;\n"),
        cylinder(d=m.nut_diameter, h=m.nut_height, center=True)
    )

    screw_positions = [
        (m.board_width/2 - m.screw_x_distance, m.board_depth/2-m.screw_y_distance),
        (-(m.board_width/2 - m.screw_x_distance),
         m.board_depth/2-m.screw_y_distance),
        (m.board_width/2 - m.screw_x_distance, -
         (m.board_depth/2-m.screw_y_distance)),
        (-(m.board_width/2 - m.screw_x_distance), -
         (m.board_depth/2-m.screw_y_distance)),
    ]

    for (shift_x, shift_y) in screw_positions:
        case -= translate((shift_x, shift_y, (case_height -
                          m.attachment_height)/2))(screw_hole)
        case -= translate((shift_x, shift_y,
                          (case_height - m.nut_height)/2))(nut_hole)

    usb = rounded_cube((m.usb_depth, m.usb_height, m.thickness), m.usb_radius)

    case -= translate((-(m.board_width+m.thickness)/2, -(m.board_depth)/2 +
                      m.usb_y_shift, case_height/2-m.usb_z_shift))(rotate((90, 0, 90))(usb))

    case = intersection()(
        case,
        raw_figure,
    )

    # Raising, rotating the case and hulling it with the base
    base_shift = (case_height*sqrt(3))/2
    base_depth = m.board_depth+m.thickness*2
    base_height = m.adhesive_tape_height + m.thickness

    adhesive_tape_depth = base_depth - 8

    standing_raw_figure = translate((0, 0, base_height))(rotate((rotation_angle, 0, 0))(
        translate((0, m.board_depth/2+m.thickness, case_height/2))(raw_figure)))

    standing_internal_raw_figure = translate((0, 0, base_height))(rotate((rotation_angle, 0, 0))(
        translate((0, m.board_depth/2, (case_height-m.thickness)/2))(internal_raw_figure)))
    standing_screen_clearing = translate((0, 0, base_height))(rotate((rotation_angle, 0, 0))(
        translate((0, m.board_depth/2+m.thickness, (case_height+m.thickness)/2))(raw_screen_clearing)))

    base = rounded_cube((m.board_width+m.thickness*2,
                         base_depth, base_height), m.board_radius)

    internal_base = translate((0, 0, -.0005))(rounded_cube((m.board_width,
                                                            base_depth-m.thickness*2, .001), m.board_radius))

    standing_case = hull()(
        standing_raw_figure,
        translate((0, base_depth/2-base_shift, base_height/2))(base)
    ) - hull()(
        standing_internal_raw_figure,
        translate((0, base_depth/2-base_shift,
                  (base_height+m.thickness)/2))(internal_base)
    )

    # Refining the base with adhesive tape clearings
    adhesive_tape = cube((m.adhesive_tape_width,
                          adhesive_tape_depth, m.adhesive_tape_height), center=True)
    standing_case -= translate((-(m.board_width-m.adhesive_tape_width)/2+m.adhesive_tape_spacing,
                               base_depth/2-base_shift, +m.adhesive_tape_height/2))(adhesive_tape)
    standing_case -= translate((+(m.board_width-m.adhesive_tape_width)/2-m.adhesive_tape_spacing,
                               base_depth/2-base_shift, +m.adhesive_tape_height/2))(adhesive_tape)

    # Moving attachment_case to position
    clearing_height = 100
    attachment_case_clearings = difference()(
        raw_figure,
        case,
    )

    # Screw clearing
    for (shift_x, shift_y) in screw_positions:
        attachment_case_clearings += translate((shift_x, shift_y, +case_height/2))(
            cylinder(d=m.screw_head_diameter, h=clearing_height))

    # Grates
    grate = build_grate(
        m.grate_width, clearing_height, m.grate_height, m.board_width - m.screw_head_diameter*4)
    attachment_case_clearings += translate((0, -(-base_depth/2+m.thickness+m.grate_height/2), case_height/2))(
        grate
    )
    lateral_grates = translate((0, base_depth-m.board_depth/2-m.grate_height/2, m.grate_height/2+base_height))(
        rotate((90, 0, 90))(
            build_grate(m.grate_width, clearing_height+m.board_width,
                        m.grate_height, m.board_depth/2, center=True)
        )
    )
    standing_case -= lateral_grates

    # Breadboard attachment
    attachment_height = m.breadboard_height/2
    breadboard_mold = cube((m.breadboard_width+m.thickness*2,
                           m.breadboard_depth+m.thickness*2, attachment_height), center=True)

    breadboard_negative = cube(
        (m.breadboard_width, m.breadboard_depth, attachment_height), center=True)
    breadboard_negative += cube((m.breadboard_notch, m.breadboard_depth +
                                m.thickness*2, attachment_height), center=True)
    breadboard_negative += cube((m.breadboard_width + m.thickness*2,
                                m.breadboard_notch, attachment_height), center=True)

    attachment_y_distance = (
        (m.breadboard_height-m.adhesive_tape_height)*math.sin(math.radians(90 - rotation_angle)))/math.sin(math.radians(rotation_angle))+1
    standing_case += translate((0, -m.breadboard_depth/2-m.thickness -
                               base_shift-attachment_y_distance+base_depth, base_height+attachment_height/2-m.adhesive_tape_height*2))(breadboard_mold)
    standing_case -= translate((0, -m.breadboard_depth/2-m.thickness -
                               base_shift-attachment_y_distance+base_depth, base_height+attachment_height/2-m.adhesive_tape_height*2))(breadboard_negative)

    # Back access port
    standing_case -= translate((0, 0, m.adhesive_tape_height+m.thickness+m.back_port_diameter/2+m.back_port_z))(
        rotate((-90, 0, 0))(cylinder(d=m.back_port_diameter, h=clearing_height))
    )
    standing_case -= standing_screen_clearing

    # Finishing touches
    attachment_case_clearings = translate((0, 0, base_height))(rotate((rotation_angle, 0, 0))(
        translate((0, m.board_depth/2+m.thickness, case_height/2))(rotate((0, 180, 0))(attachment_case_clearings))))

    attachment_case = translate((0, 0, base_height))(rotate((rotation_angle, 0, 0))(
        translate((0, m.board_depth/2+m.thickness, case_height/2))(rotate((0, 180, 0))(case))))

    result = attachment_case + standing_case - attachment_case_clearings

    return result


def main():
    m = Measures()
    final = piece(m)

    scad_render_to_file(scad_inline("\n$fn=32;\n") + final, "case.scad")


if __name__ == "__main__":
    main()
