include <keycap_frame.scad>

difference() {
  union() {
    body();
    buttons();
  }
  //pcb();
}

translate([50,91,2.6]) scale([1.5,1.5,1]) import("beatmania_iidx.stl");

%color("gray", 0.3) pcb();

module pcb() {
  translate([-137.32,144.4,-1.21]) import("iidx_pico_to_stl v1.stl");
}

module body() {
  difference() {
    union() {
      color("green") translate([0,45,0]) rcube(183,163,3,6.5);
    }
    // main button holes
    color("#2C3E50") {
      translate([-61.5,0]) main_hole();
      translate([-20.5,0]) main_hole();
      translate([20.5,0]) main_hole();
      translate([61.5,0]) main_hole();
      translate([-41,56]) main_hole();
      translate([0,56]) main_hole();
      translate([41,56]) main_hole();
    }
    // aux button holes
    color("#2C3E50") {
      translate([-65,110]) aux_hole();
      translate([-32,110]) aux_hole();
      translate([1,110]) aux_hole();
      translate([34,110]) aux_hole();
    }
    // set button holes
    color ("darkgray") translate([0,0,-1]) {
      hull() {
        translate([59,110]) rcube(7,8.8,2.5,1);
        translate([59,110]) rcube(7,7,4.1,2);
      }
      translate([59,110]) rcube(7,7,6,2);

      hull() {
        translate([70,110]) rcube(7,8.8,2.6,1);
        translate([70,110]) rcube(7,7,4.1,2);
      }
      translate([70,110]) rcube(7,7,6,2);
    }
    // pogo holes
    translate([-48.92,121.5,0]) rotate([0,0,90]) pogo_hole();
    translate([86.6,74.42,0]) rotate([0,0,0]) pogo_hole();
    // usb hole
    translate([-16.72,121.4]) usb_hole();

    // screws
    color("gray") translate([0,0,-1],$fn=24) {
      translate([-82.5,117.5]) cylinder(d=3.4,h=10);
      translate([-82.5,-27.5]) cylinder(d=3.4,h=10);
      translate([82.5,117.5]) cylinder(d=3.4,h=10);
      translate([82.5,-27.5]) cylinder(d=3.4,h=10);
    }
      
    // screw sinks
    color("gray") translate([0,0,1.5],$fn=24) {
      translate([-82.5,117.5]) cylinder(d=6,h=10);
      translate([-82.5,-27.5]) cylinder(d=6,h=10);
      translate([82.5,117.5]) cylinder(d=6,h=10);
      translate([82.5,-27.5]) cylinder(d=6,h=10);
    }
  }
}

module main_hole()
{
  cube([24.2,41.2,20],center=true);
}

module aux_hole()
{
  cube([18,18,20],center=true);
}

module pogo_hole()
{
  rotate([90,0,0]) resize([3,4.5,14]) cylinder(d=1,h=1,center=true,$fn=32);
}

module usb_hole()
{
  translate([4.3,0]) rotate([90,0,0]) resize([1.6,2,6]) cylinder(d=1,h=1,center=true,$fn=32);
  translate([-4.3,0]) rotate([90,0,0]) resize([1.6,2,6]) cylinder(d=1,h=1,center=true,$fn=32);
}

module buttons() {
  // main button
  color("#2C3E50") translate([0,0,2.99]) {
    translate([-61.5,0]) main_button();
    translate([-20.5,0]) main_button();
    translate([20.5,0]) main_button();
    translate([61.5,0]) main_button();
    translate([-41,56]) main_button();
    translate([0,56]) main_button();
    translate([41,56]) main_button();
  }
  // aux button
  color("#2C3E50") translate([0,0,2.99]) {
    translate([-65,110]) aux_button();
    translate([-32,110]) aux_button();
    translate([1,110]) aux_button();
    translate([34,110]) aux_button();
  }
   // set button
  color ("#2C3E50") translate([0,0,2.9]) {
    difference() {
      hull() {
        translate([59,110]) rcube(8.5,8.5,1,2);
        translate([59,110]) rcube(10,10,0.1,2.75);
      }
      translate([59,110,-1]) rcube(7,7,6,2);
    }
    difference() {
      hull() {
        translate([70,110]) rcube(8.5,8.5,1,2);
        translate([70,110]) rcube(10,10,0.1,2.75);
      }
      translate([70,110,-1]) rcube(7,7,6,2);
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