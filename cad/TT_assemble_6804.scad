import("TT_base_150.stl");
translate([0,0,20]) color("cyan") rotate([0,0,60]) import("bearing_seat_6804.stl");
translate([0,0,15]) color("silver") rotate([0,90,0]) resize([7,32,32]) import("nsk6804.stl");
translate([0,0,34]) color("lime") rotate([0,180,0]) import("TTshaft_6804.stl");

translate([0,0,40]) color("black", 0.3) cylinder(d=150,h=4,$fn=128);

translate([97.5,191.5,2.8]) color("green") rotate([0,0,-90]) import("iidx_tt v1.stl");
