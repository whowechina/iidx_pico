sd=6; 
ss=2.9;

shaft6804();
//shaft6805();

%magnet();

module shaft6804() {
  difference() {
    union($fn=256) {
      cylinder(d=20.1,h=10.3);
      cylinder(d=24,h=3.3);
    }
    
    union($fn=128) {
      translate([0,0,9.8]) cylinder(d=18,h=3);
      color("cyan") translate([0,0,7.8]) cylinder(d=6.3,h=4); // magnet
    }

    color("gray") for (i=[0:90:270]) {
      hole(i);
    }  
  }

  %nsk6804();
}

module shaft6805() {
  difference() {
    union($fn=256) {
      cylinder(d=25.1,h=10.3);
      cylinder(d=29,h=3.3);
    }
    
    union($fn=128) {
      translate([0,0,9.8]) cylinder(d=22,h=3);
      color("cyan") translate([0,0,7.8]) cylinder(d=6.3,h=4); // magnet
    }

    color("gray") for (i=[0:90:270]) {
      hole(i);
    }  
  }

  %skf6805();
}

module hole(angle) {
  $fn=64;
  x = cos(angle)*sd;
  y = sin(angle)*sd;
  translate([x,y,-1]) cylinder(d=ss,h=14);
  hull() {
    translate([x,y,-0.1]) cylinder(d=ss,h=2);
    translate([x,y,-0.1]) cylinder(d=ss+1.2,h=0.1);
  }
}

module nsk6804() {
  translate([0,0,3.3]) rotate([0,90,0]) resize([7,32,32]) import("nsk6804.stl");
}

module skf6805() {
  translate([0.72,1.93,9.5]) rotate([0,90,0]) import("skf6805.stl");
}

module magnet() {
  translate([0,0,8]) cylinder(d=6,h=2,$fn=64);
}
