pogo=true; // if you use pogopin connector
jack35=true; // if you use 3.5mm headphone jack

body();
stab();

%color("cyan", 0.3) pcb();

module pcb() {
  translate([-137.32,144.4,5]) import("iidx_pico v1.stl");
}

module body() {
  difference() {
    color("green", 0.8) translate([0,45,0]) rcube(183,163,6.4,6.5);
    color("darkgreen") translate([0,45,5]) rcube(180.3,160.3,10,5.15);

    // main button
    color("red") translate([0,0,2]) {
      translate([-61.5,0]) rcube(20,37,10,2);
      translate([-20.5,0]) rcube(20,37,10,2);
      translate([20.5,0]) rcube(20,37,10,2);
      translate([61.5,0]) rcube(20,37,10,2);
      translate([-41,56]) rcube(20,37,10,2);
      translate([0,56]) rcube(20,37,10,2);
      translate([41,56]) rcube(20,37,10,2);
    }

    // main button area holes
    color("brown") translate([0,0,2]) {
      translate([-45,28]) rcube(84,12,10,2);
      translate([45,28]) rcube(84,12,10,2);
      translate([0,-27]) rcube(146,12,10,2);

      translate([-80.5,0]) rcube(14,37,10,2);
      translate([-41,0]) rcube(17,37,10,2);
      translate([0,0]) rcube(17,37,10,2);
      translate([41,0]) rcube(17,37,10,2);
      translate([80.5,0]) rcube(14,37,10,2);
      
      translate([-70,56]) rcube(34,37,10,2);
      translate([-20.5,56]) rcube(17,37,10,2);
      translate([20.5,56]) rcube(17,37,10,2);
      translate([67.5,56]) rcube(28.5,37,10,2);
    }
    
    // pi pico area holes
    color("navy") translate([0,0,1.5]) {
      translate([40,87.5,0]) rcube(84,23,10,2);
      translate([-46,87.5,0]) rcube(82,23,10,2);
      translate([-29,84,-3]) cylinder(d=3,h=10,$fn=32); // reset button
    }
    
    // upper button area holes
    color("navy") translate([0,0,1.2]) {
      translate([-32,111,0]) rcube(84,20,10,2);
      translate([35,111,0]) rcube(19,20,10,2);
      translate([60,111,0]) rcube(25,20,10,2);
    }
    
    if (pogo) {
      color("cyan") translate([0,0,5.7]) {
        translate([-48.92,139,0]) rotate([90,0,0]) rcube(20.5,10,20,0.1);
        translate([84,74.42,0]) rotate([90,0,90]) rcube(20.5,10,20,0.1);
      }
    }
    
    if (jack35) {
      color("cyan") translate([0,0,5.7]) {
        translate([17.8,139,0]) rotate([90,0,0]) rcube(6.8,12,28.6,0.2);
        translate([17.8,125.1,0]) rotate([90,0,0]) rcube(10,3,13,0.1);
        translate([17.8,128,-7]) cylinder(d=12,h=10,$fn=4);
      }
    }
  
    // usb
    color("cyan") translate([0,0,6.7]) {
      translate([-16.72,139,0]) rotate([90,0,0]) rcube(9.5,10,20,1);
    }
    
    // pads
    color("pink") translate([0,0,-1],$fn=48) {
      translate([-82.5,117.5]) cylinder(d=13.8,h=2.2);
      translate([-82.5,-27.5]) cylinder(d=13.8,h=2.2);
      translate([82.5,117.5]) cylinder(d=13.8,h=2.2);
      translate([82.5,-27.5]) cylinder(d=13.8,h=2.2);
      translate([0,28]) cylinder(d=13.8,h=2.2);
    }
    
    // screws
    color("darkred")  translate([0,0,-1],$fn=24) {
      translate([-82.5,117.5]) cylinder(d=3.3,h=10);
      translate([-82.5,-27.5]) cylinder(d=3.3,h=10);
      translate([82.5,117.5]) cylinder(d=3.3,h=10);
      translate([82.5,-27.5]) cylinder(d=3.3,h=10);

      union($fn=6) {
        translate([-82.5,117.5]) cylinder(d=6.7,h=4.5);
        translate([-82.5,-27.5]) cylinder(d=6.7,h=4.5);
        translate([82.5,117.5]) cylinder(d=6.7,h=4.5);
        translate([82.5,-27.5]) cylinder(d=6.7,h=4.5);
      }
    }
  }
}

module stab() {
  {
    button_stab(-61.5,0);
    button_stab(-20.5,0);
    button_stab(20.5,0);
    button_stab(61.5,0);
    button_stab(-41,56);
    button_stab(0,56);
    button_stab(41,56);
  }
}

module button_stab(x, y) {
   translate([x,y,1]) {
    difference() {
      color("purple") translate([-3.5,7.9,0]) cube([8.6,11,2.85]);
      color("pink") translate([-2,9.4,1.35]) cube([5.6,10,4]);
    }
    difference() {
      color("purple") translate([-3.5,-20,0]) cube([8.6,12.1,2.85]);
      color("pink") translate([-2,-20,1.35]) cube([5.6,10.6,4]);
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