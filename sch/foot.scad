// A simple "foot" for C64 keyboards.
// There were many versions of this keyboard.
// This design may not work on all of them.

$fn = 32;

difference() {
    block();

    translate([-1,-1,9])
    rotate([7,0,0])
    cube([22,150,30]);

    rotate([7,0,0])
    translate([10,24,8])
    cylinder(3, d=11);

    rotate([7,0,0])
    translate([10,117,8])
    cylinder(3, d=11);
}

module block() {
    difference() {
        translate([0,14,0])
        cube([20,35,8.8]);

        translate([-1,34,0])
        rotate([-60,0,0])
        cube([22,20,20]);
    }

    difference () {

        translate([0,14,7.8])
        cube([20,112,30]);

        translate([10,120,0])
        cylinder(7.8+7.5, d=3.4);

    };
}
