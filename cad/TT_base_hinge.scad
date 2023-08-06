export_tt = false; // this is for main body
export_hinge = true; // actually this is for hinge cover

disc_d=170; // disc size you need to specify


d1=disc_d+14; // lower outer
d2=disc_d+4; // upper outer
d3=disc_d+1.2; // disc area
led=disc_d-8; // led ring
hinge_offset=disc_d/2+5;

h1=6; // base
h2=15; // top
h3=11; // bed
h5=3; // bearing to pcb clearance

led_w=18;

led_bed=3;
bearing_seat=4;

d5=28; // bearing to pcb clearance

pcb_h=1.3;

$fn=64;

if (export_tt) body(); 
if (export_hinge) {
  hinge_top();
  translate([hinge_offset+6.3,0,8.7]) rotate([0,0,-90]) color("DimGray")
    linear_extrude(height = 1) text("pico", font="Bookman Old Style:style=Bold", size=6.7, valign="center", halign="center");
}

module hinge_top() {
  difference() {
    translate([hinge_offset-2,0,6]) rcube(29,60,3,5);
    translate([0,0,6]) {
      screw_top([hinge_offset+5,-24]);
      screw_top([hinge_offset+5,24]);
    }
    hull() {
      cylinder(d=d1+0.2, h=h1, $fn=256);
      cylinder(d=d2+0.2, h=h2, $fn=256);
    }
  }
}

module body() {
  difference() {
    color("gray") {
      hull() {
        cylinder(d=d1, h=h1, $fn=256);
        cylinder(d=d2, h=h2, $fn=256);
      }
      translate([hinge_offset,0,0]) rcube(25,60,6,5);
    }

    color("gray") translate([0, 0, h3]) cylinder(d=d3, h=90, $fn=256);
    color("gray") translate([0, 0, h5]) cylinder(d=d5, h=90);
    
    color("green") translate([0,0, led_bed]) difference() {
      cylinder(d=led,h=90,$fn=128);
      translate([0,0,-1]) cylinder(d=70,h=92,$fn=128);
    }
    
    color("pink") translate([0, 0, bearing_seat]) cylinder(d=led, h=90, $fn=256);

    // bearing screws
    color("gold") {
      screw([-14.5,-14.5,-1.81]);
      screw([14.5,14.5,-1.81]);
      screw([-14.5,14.5,-1.81]);
      screw([14.5,-14.5,-1.81]);
    }
    color("darkgreen") translate([0,0,pcb_h]) rcube(23.2,23.2,5,2.5);
    
    // pads
    for (i = [90:60:300]) {
      pr=d2/2-2;
      translate([cos(i)*pr, sin(i)*pr, -1]) cylinder(d=9.8,h=1.8);
    }
    
    hinge();
    cable();
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

module screw(pos) {
  translate(pos) {
    cylinder(d=3.3,h=10,$fn=32);
    cylinder(d=6.9,h=4.4,$fn=6);
    cylinder(d=9.8,h=1.8,$fn=32);
  }
}

module screw_top(pos) {
  color("gray") translate(pos) {
    translate([0,0,-1],$fn=24) cylinder(d=3.4,h=10);
    translate([0,0,1.5],$fn=24) cylinder(d=6,h=10);
 }
}

module cable() {
  translate([10,2,3]) rotate([0,90,0]) cylinder(d=3.4,h=hinge_offset+5);
  translate([10,-2,3]) rotate([0,90,0]) cylinder(d=3.4,h=hinge_offset+5);
  translate([10,-2,1.5]) cube([hinge_offset-19.2,4,4]);
}

module hinge() {
    // TT hinge junction
    color("gray") translate([5,0]) {
      translate([hinge_offset+7,0,3]) rcube(8,41,4,0.1);
      difference() {
        translate([hinge_offset,0,1.5]) rcube(9,41,8,0.1);
        translate([hinge_offset+0.5,-10,1.5]) rcube(3.5,8,5,1);
        translate([hinge_offset+0.5,10,1.5]) rcube(3.5,8,5,1);
      }      
      wire_clamp([hinge_offset+16,0,3]);
    }

    translate([hinge_offset-2.5,0,0]) hull() {
      translate([4,-20.5,9]) cube([2,41,0.1]);
      translate([0,-20.5,1.5]) cube([6,41,0.1]);
    }


  color("pink")  translate([5,0,-1]) {
    screw([hinge_offset,-24]);
    screw([hinge_offset,24]);
  }
}

module wire_clamp(pos) {
  translate(pos) rotate([0,-90,0]) hull($fn=64) {
    translate([1,-1.2]) cylinder(d=6,h=40,center=true);
    translate([1,1.2]) cylinder(d=6,h=40,center=true);
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

module nsk6804() {
  translate([0,0,h4]) rotate([0,90,0]) resize([7,32,32]) import("nsk6804.stl");
}
