h1=4; // base
h2=17; // top
h3=12.5; // bed
h4=5.3; // bearing
h5=3; // pcb top

led_h=4;

// 150 disc
disc_d=150;
d1=175; // lower outer
d2=155; // upper outer
d3=152; // disc area
led=142; // led ring

// 170 disc
//disc_d=170;
//d1=195; // lower outer
//d2=175; // upper outer
//d3=172; // disc area
//led=160; // led ring


// 180 disc
/*
disc_d=180;
d1=205; // lower outer
d2=185; // upper outer
d3=182; // disc area
led=168; // led ring
*/

d4=58.2; // bearing
d5=28; // pcb

pcb_h=1.3;
d6=75;

$fn=64;

body();
//%disc();
%nsk6804();
%pcb();

module body() {
  difference() {
    color("gray", 0.8) hull() {
      cylinder(d=d1, h=h1, $fn=256);
      cylinder(d=d2, h=h2, $fn=256);
    }
    color("gray") translate([0, 0, h3]) cylinder(d=d3, h=90, $fn=256);
    color("gold") translate([0, 0, h4]) cylinder(d=d4, h=90, $fn=128);
    color("gray") translate([0, 0, h5]) cylinder(d=d5, h=90);
    
    color("pink") translate([0, 0, led_h]) difference() { cylinder(d=led, h=90, $fn=256);
      cylinder(d=d6, h=90);
    }

    color("purple") {
      translate([0,22,-1]) cylinder(d=4.2,h=90);
      translate([11*sqrt(3),-11,-1]) cylinder(d=4.2,h=90);
      translate([-11*sqrt(3),-11,-1]) cylinder(d=4.2,h=90);
    }
    color("dimgray") {  
      translate([0,22,-0.1]) cylinder(d=10,h=2.2);
      translate([11*sqrt(3),-11,-0.1]) cylinder(d=10,h=2.2);
      translate([-11*sqrt(3),-11,-0.1]) cylinder(d=10,h=2.2);
    }
   
    color("darkgreen") translate([0,0,pcb_h]) rcube(23.2,23.2,5,2.5);

    color("green") hull() {
      translate([-5, -15, pcb_h]) cube([10, 5, 25]);
      translate([-3, -56, pcb_h-0.3]) cube([6, 10, 25]);
    }
    translate([-15, -60, led_h+0.1]) cube([30, 20, 10]);
    color("cyan") translate([0,-50,3.5]) rotate([90,0,0]) cylinder(d=5,h=150);
    
    // pads
    for (i = [30:60:330]) {
      pr=d2/2-1;
      translate([sin(i)*pr, cos(i)*pr, -1]) cylinder(d=13.8,h=2.5);
    }
  }
}

module rcube(w,h,d,r) {
  hull($fn=r*16) {
    translate([w/2-r,-h/2+r,0]) cylinder(r=r,h=d);
    translate([-w/2+r,h/2-r,0]) cylinder(r=r,h=d);
    translate([w/2-r,h/2-r,0]) cylinder(r=r,h=d);
    translate([-w/2+r,-h/2+r,0]) cylinder(r=r,h=d);
  }
}

module disc() {
  disc_h=16;
  disc_th=4;
  color("darkblue", 0.3) hull () {
    translate([0,0,disc_h]) cylinder(d=disc_d-2,h=disc_th);
    translate([0,0,disc_h]) cylinder(d=disc_d,h=disc_th-1);
  }
}

module pcb() {
  translate([97.5,191.5,3]) rotate([0,0,-90]) import("iidx_tt v1.stl");
}

module nsk6804() {
  translate([0,0,h4]) rotate([0,90,0]) resize([7,32,32]) import("nsk6804.stl");
}
