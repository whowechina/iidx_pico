seat6804();
//translate([60,0,0]) seat6805();

module seat6804() {
  difference($fn=256) {
    union() {
      cylinder(d=55,h=8.7); // brim
    }
    translate([0,0,-1]) cylinder(d=32.1,h=7.5); // bearing
    translate([0,0,6.49]) cylinder(d1=32.1,d2=30,h=1.02); // limiter
    translate([0,0,-1]) cylinder(d=30,h=20); // limiter
    
    for (a=[30:120:270],$fn=64) {
      x = cos(a)*22;
      y = sin(a)*22;
      translate([x,y,-1]) cylinder(d=4.2,h=11);
      translate([x,y,6]) cylinder(d=8.3,h=11,$fn=6);
    }
  }
  %nsk6804();
}

module seat6805() {
  difference($fn=256) {
    union() {
      //cylinder(d=38.4,h=9);
      cylinder(d=55,h=8.7); // brim
    }
    translate([0,0,-1]) cylinder(d=37.1,h=7.5); // bearing
    translate([0,0,6.49]) cylinder(d1=37.1,d2=35,h=1.02); // limiter
    translate([0,0,-1]) cylinder(d=35,h=20); // limiter
    
    for (a=[30:120:270],$fn=64) {
      x = cos(a)*22;
      y = sin(a)*22;
      translate([x,y,-1]) cylinder(d=4.2,h=11);
      translate([x,y,5.7]) cylinder(d=8.3,h=11,$fn=6);
    }
  }
  %skf6805();
}

module nsk6804() {
  rotate([0,90,0]) resize([7,32,32]) import("nsk6804.stl");
}

module skf6805() {
  translate([0.72,1.93,6.2]) rotate([0,90,0]) import("skf6805.stl");
}
