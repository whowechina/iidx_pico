t=10;
w=23;
h=40;
r=0.8;
solid=1.1;
zoff=-0.3;

gap_r = 0.18; // radius of round corner in cross
stem_l = 4.85; // socket stem length
stem_d = 5.6; // socket stem diameter

translate([0,0,0]) {
  cap();
  *stems(); // Choc V1, SLA
  *stems(v1=false); // Choc V2, SLA
  stems(v1=false,sla=false); // Choc V2, FDM
}


%union() {
  mark([0,2.3,0],3.3,"purple");
  mark([0,12,0],3.7,"orange");
  mark([1,10,0],0.8,"cyan");
  mark([0,0,0],6.6,"red");
}

module mark(offset=[0,0,0],h=5.5,c="red") {
  translate(offset) {
    color(c) hull() {
      cylinder(d=0.1,h=h,$fn=32);
      cylinder(d=1,h=0.1,$fn=64);
    }
    color("white") cylinder(d=0.1,h=h+5,$fn=64);
  }
}

module stems(v1=true,sla=true) {
  if (sla) {
    // for SLA printing
    translate([0,0,1.2]) {
      stab_stem([0,12,0],stem_l,5.6,1.04,4.14,4.36,3.5);
      stab_stem([0,-12,0],stem_l,5.6,1.04,4.14,4.36,3.5);
    }
  } else {
    // for FDM printing
    translate([0,0,1.2]) {
      stab_stem([0,12,0],stem_l,5.7,1.1,4.2,4.4,3.5);
      stab_stem([0,-12,0],stem_l,5.7,1.1,4.2,4.4,3.5);
    }
  }
  if (v1) {
    // choc v1
    translate([0,0,1.4]) rotate([0,0,90]) color("cyan") {
      choc_stem([0,2.85,0],4.5);
      choc_stem([0,-2.85,0],4.5);
    }
  } else {
    // choc v2
    translate([0,0,1.8]) {
      stab_stem([0,0,0],stem_l-1.8,5.48,1.31,4.02,4.02,5);
    }
  }
}

module cap() {
  difference() {
    body(w,h,r);
    translate([0,0,-solid]) body(w-2*solid,h-2*solid,r);
  }
}

module body(w,h,r) {
  difference() {
    translate([0,0,zoff-0.5]) minkowski() {
      intersection() {
        ww=w-2*r;
        hh=h-2*r;
        translate([-ww/2,-hh/2,0]) rcube(ww,hh,20,0.7,$fn=64);
        translate([0,0,-5]) resize([50,39,26]) rotate([90,0,0]) cylinder(d=10,h=10,center=true,$fn=256);
      }
      sphere(r=r,$fn=48);
    }
    translate([-50,-50,-10]) cube([100,100,10]);
  }
}

module rcube(x,y,z,r) {
  hull() {
    translate([r,r,0]) cylinder(r=r,h=z);
    translate([x-r,r,0]) cylinder(r=r,h=z);
    translate([r,y-r,0]) cylinder(r=r,h=z);
    translate([x-r,y-r,0]) cylinder(r=r,h=z);
  }
}

module choc_stem(offset=[0,0,0],h) {
  translate([0,0,6.4+zoff-h]) translate(offset) {
    translate([-1.5,-0.6,0.3]) cube([3,1.2,h-0.4]);
    hull() {
      translate([-1.5,-0.6,0.3]) cube([3,1.2,0.1]);
      translate([-1.2,-0.5,0.0]) cube([2.4,1,0.1]);
    }
    hull() {
      translate([-1.5,-0.6,h-1.2]) cube([3,1.2,0.1]);
      translate([-2.5,-1.5,h-0.1]) cube([5,3,0.1]);
    }
  }
}

module stab_stem(offset=[0,0,0], length, d, gap_w, gap_h1, gap_h2,dig) {
  translate([0,0,zoff]) translate(offset) difference() {
    translate([0,0,5.2-length]) {
      cylinder(d=d,h=length,$fn=64);
      translate([0,0,length-0.01]) cylinder(d1=d,d2=8,h=1,$fn=64);
      translate([0,0,-0.4]) cylinder(d1=d-0.7,d2=d,h=0.401,$fn=64);
    }
    translate([0,0,3.5-length]) difference() {
      l=length+1;
      union() {
        translate([0,0,l/2]) color("green") cube([gap_h1,gap_w,dig], center=true);
        translate([0,0,l/2]) color("red") cube([gap_w,gap_h2,dig], center=true);
        translate([0,0,l/2]) cube([gap_w+gap_r*2,gap_w+gap_r*2,dig], center=true);
      }
      
      r = gap_w/2 + gap_r;
      for (i=[-1,1]) {
        for (j=[-1,1]) {
          translate([i*r,j*r,-1]) cylinder(r=gap_r,h=6,$fn=24);
        }
      }
    }
  }
}

