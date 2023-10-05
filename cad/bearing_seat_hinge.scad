seat6804();

module seat6804() {
  difference($fn=256) {
    union() {
      cylinder(d=50,h=4); // brim
      cylinder(d=36,h=8);
    }
    translate([0,0,-1]) cylinder(d=32.1,h=7.3); // bearing
    translate([0,0,6.29]) cylinder(d1=32.1,d2=30,h=1.2); // limiter
    translate([0,0,-1.2]) cylinder(d=30,h=20); // limiter
    screws();
  }
  *%nsk6804();
}


module screws() {
  screw([-14.5,-14.5]);
  screw([14.5,14.5]);
  screw([-14.5,14.5]);
  screw([14.5,-14.5]);
}

module screw(pos) {
  translate(pos) {
    translate([0,0,-1]) cylinder(d=3.4,h=11);
    translate([0,0,2.5]) cylinder(d=6,h=11,$fn=32);
  }
}

module nsk6804() {
  rotate([0,90,0]) resize([7,32,32]) import("nsk6804.stl");
}
