// A simple "foot" for C64 keyboards.
// There were many versions of this keyboard.
// This design may not work on all of them.

$fn = 32;

difference() {
    block();

    translate([-1,-1,15])
    rotate([7,0,0])
    cube([22,150,30]);

    rotate([7,0,0])
    translate([10,17,13.5])
    cylinder(3, d=13);

    rotate([7,0,0])
    translate([10,115,13.5])
    cylinder(3, d=13);
}

module block() {
    difference () {

        translate([0,15,0])
        cube([20,96,8.8]);

        translate([-1,25,-1])
        cube([22,76,8.8]);
    };

    difference () {

        translate([0,0,7.8])
        cube([20,126,30]);

        translate([10,6,0])
        cylinder(7.8+7.5, d=3.4);

        translate([10,120,0])
        cylinder(7.8+7.5, d=3.4);

    };
}
