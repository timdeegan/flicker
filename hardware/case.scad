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

// Leave this much room on each flat surface for assembly.
ext = 0.1;

// 
// Holder for the phototransistor.
// 
// For now this is a simple box but later it will be part of the
// case.  The PT fits into a cylindrical sleeve, and that sleeve
// fits into a hole in the top of the box, with the filter below it.

// Size of the main body of the PT (excluding the lip).
pt_radius = 2.5;

// Thickness of the IR/UV filter.
filter_height = 1;
// Radius of the IR/UV filter.
filter_radius = 5;

// Minimum wall thickness in the holder.
holder_wall = 2;

// Width of the finished holder.
holder_width = 20;
// Height of the finished holder.
holder_height = 30;

// A hole to press-fit a cylinder into.
module hole(height, radius) {
    // If we make a cylindrical hole then we'll need to
    // mess around with tolerances to match the printer.
    // Instead make a slightly tapered hole with 'crush ribs'
    // that will squash out to accomodate the other part.
    // (TBH not sure this matters at the size I'm using.)
    clearance = height * sin(2);
    smaller_radius = radius;
    larger_radius = smaller_radius + clearance;
    ribs = 6;
    difference() {
        // The tapered hole itself.
        cylinder(h=height, r1=larger_radius, r2=radius);
        // Ribs come in to the nominal radius.
        for (n = [0:ribs]) {
            translate([(larger_radius) * sin(360 * n / ribs),
                       (larger_radius) * cos(360 * n / ribs), 0])
                cylinder(h=height, r=clearance);
        }
    }
}

// Inner sleeve that holds the PT.
module pt_sleeve() {
    height = (holder_height
              - (holder_wall + filter_height)
              - ext);
    difference() {
        cylinder(h=height, r=filter_radius);
        hole(height, pt_radius);
    }
}

// Outer body of the holder, that the sleeve fits into.
module pt_holder()
{
    hole_radius = filter_radius;
    hole_height = holder_height - holder_wall + ext;

    difference() {
        // Outer shell, a box.
        translate([-holder_width / 2, -holder_width / 2]) 
            cube([holder_width, holder_width, holder_height]);
        // Hole right through for light to pass.
        cylinder(h=holder_height, r=pt_radius);
        // Hole at the top for the sleeve to fit into.
        translate([0, 0, holder_height])
            mirror([0, 0, 1])
                hole(hole_height, hole_radius);
    }
}

// Lay all the components out as they will be printed.
module everything() {
    pt_holder();
    translate([30, 0, 0]) pt_sleeve();
}

everything();
