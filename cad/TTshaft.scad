magnet=[6,2]; // define your magnet: [diameter, thickness]
sd=6; 
ss=2.95;

//shaft6804();
shaft6805();

%magnet();


module shaft6804() {
  difference() {
    union($fn=256) {
      cylinder(d=20.2,h=10.3);
      cylinder(d=24,h=3.3);
    }
    translate([0,0,10]) cylinder(d=18,h=3,$fn=64);
    core();
  }

  %nsk6804();
}

module shaft6805() {
  difference() {
    union($fn=256) {
      cylinder(d=25.2,h=10.3);
      cylinder(d=29,h=3.3);
    }
    translate([0,0,10]) cylinder(d=22,h=3,$fn=64);
    core();
  }

  %skf6805();
}

module core() {
  color("cyan") translate([0,0,10-magnet[1]]) cylinder(d=magnet[0]+0.3,h=10,$fn=64); // magnet

  color("gray") for (i=[0:90:270]) {
    hole(i);
  }  
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
  translate([0,0,10.2-magnet[1]]) cylinder(d=magnet[0],h=magnet[1],$fn=64);
}
