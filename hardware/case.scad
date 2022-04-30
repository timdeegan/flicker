//
// 3D-printed case for flicker meter.
//
// Use OpenSCAD to convert this to STL and then whatever
// slicing tool your 3D printer needs.
//

// OpenSCAD makes arcs by assembling straight segments of
// length >=2, covering <= 12 degrees, but that gets chunky for
// smaller parts, and opposite sides might not be parallel.
// Render circles with this many segments.
$fn = 40;
// OpenSCAD approximates circles by fitting polygons inside them.
// To get a polygon *outside* the circle, scale up by this amount.
outer_circle_ratio = 1 / cos(180/$fn);

// Leave this much room on each surface for assembly.
ext = 0.1;

// 
// Holder for the phototransistor.
// 
// For now this is a simple box but later it will be part of the
// case.  The PT fits into a cylindrical sleeve, and that sleeve
// fits into a hole in the top of the box, with the filter below it.

// Size of the main body of the PT (excluding the lip).
pt_diameter = 5.1;

// Thickness of the IR/UV filter.
filter_height = 1;
// Diameter of the IR/UV filter.
filter_diameter = 10;

// Minimum wall thickness in the holder.
holder_wall = 2;

// Width of the finished holder.
holder_width = 20;
// Height of the finished holder (not including the sleeve).
holder_height = 20;

// The sleeve sticks out this far, so we can remove it again.
extra_sleeve_height = 10;

// Inner sleeve that holds the PT.
module pt_sleeve() {
    height = (holder_height + extra_sleeve_height
              - (holder_wall + filter_height)
              - ext);
    outer_diameter = filter_diameter - 2 * ext;
    inner_diameter = pt_diameter * outer_circle_ratio + 2 * ext;

    difference() {
        cylinder(h=height, d=outer_diameter);
        cylinder(h=height, d=inner_diameter);
    }
}

// Outer body of the holder, that the sleeve fits into.
module pt_holder()
{
    hole_diameter = filter_diameter * outer_circle_ratio + 2 * ext;
    hole_height = holder_height - holder_wall + ext;

    difference() {
        // Outer shell, a box.
        translate([-holder_width / 2, -holder_width / 2]) 
            cube([holder_width, holder_width, holder_height]);
        // Hole right through for light to pass.
        cylinder(h=holder_height, d=pt_diameter);
        // Hole at the top for the sleeve to fit into.
        translate([0, 0, holder_height - hole_height]) 
            cylinder(h=hole_height, d=hole_diameter);
    }
}

// Lay all the components out as they will be printed.
module everything() {
    pt_holder();
    translate([30, 0, 0]) pt_sleeve();
}

everything();
