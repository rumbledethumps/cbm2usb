// A simple "edge" for C64 keyboards.
// There were many versions of this keyboard.
// This design may not work on all of them.

$fn = 64;

difference() {
     length = 150.5;

    block(length);
    cutter(length);

    translate([6.25,2.5,34])
    rotate([90,0,0])
    cylinder(h=6, d=10.5);

    translate([6.25,3.0,34])
    rotate([90,0,0])
    cylinder(h=6, d=10);

    translate([6.25,3.5,34])
    rotate([90,0,0])
    cylinder(h=6, d=9);

    translate([6.25,2.5,length-34])
    rotate([90,0,0])
    cylinder(h=6, d=10.5);

    translate([6.25,3.0,length-34])
    rotate([90,0,0])
    cylinder(h=6, d=10);

    translate([6.25,3.5,length-34])
    rotate([90,0,0])
    cylinder(h=6, d=9);

}

translate([50,0,0])
difference() {
    length = 385.5;
    block(length/2);
    cutter(length/2);
}

module cutter(length) {
    difference() {
        translate([-1,-1,-1])
        cube([16,25,length+2]);

        translate([14.5,0,-2])
        scale([1,1.29,1])
        cylinder(h=length+4, d=29);
    }
}

module block(length) {
    linear_extrude(height = length, center = false,
        convexity = 10, twist = 0)
    translate([0, 0, 0])
    polygon([[0,0],[12.5,0],[12.5,9.5],[14.5,18.7],[0,18.7]],
        paths=[[0,1,2,3,4]]);
}
