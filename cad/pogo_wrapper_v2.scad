$fn=128;

bottom();
//top();

// for demo
//translate([0,-6.1,0]) part_b();
//%translate([11.5,6,3]) import("pogo5f.stl");

//%pogo();

//color("pink") sleeve();

module bottom() {
  difference() {
    body();
    translate([-50,-20,4]) cube([100,40,30]);

    translate([38,1.25,3]) cylinder(d=1.7,h=2);
    translate([38,6.75,3]) cylinder(d=1.7,h=2);
    translate([24,6.25,3]) cylinder(d=1.7,h=2);
    translate([-0.75,4,3]) cylinder(d=1.7,h=2);
  }
  
  // pogo limiter
  translate([3,0.5,0.5]) cube([16,4.5,1.4]);
}

module top() {
  intersection() {
    body();
    translate([-50,-20,4.1]) cube([100,40,20]);
  }
  translate([38,1.25,3.2]) cylinder(d=1.5,h=2);
  translate([38,6.75,3.2]) cylinder(d=1.5,h=2);
  translate([24,6.25,3.2]) cylinder(d=1.5,h=2);
  translate([-0.75,4,3.2]) cylinder(d=1.5,h=2);

  // pogo limiter
  translate([3,3.5,4.5]) cube([16,1.5,0.8]);
}

module pogo() {
  translate([1.1,7,0.85]) rotate([90,0,0]) cube([20.2,4.3,1.5]);
  translate([11.2,9,3]) rotate([90,0,0]) hull() {
    translate([-10+2.15,0]) cylinder(d=4.3,h=4);
    translate([10-2.15,0]) cylinder(d=4.3,h=4);
  }
}

module body() {
  difference() {
    color("cyan", 0.8) rotate([-90,0,0]) hull() {
      l = 40; h = 6; d = 8; r = 1;
      translate([r, -h+r, 0]) cylinder(r=r,h=d);
      translate([r, -r, 0]) cylinder(r=r,h=d);
      translate([l-h/2, -h/2, 0]) cylinder(d=h,h=d);
      translate([0,-3,4]) sphere(d=6);
    }

    difference() {
      // cabin
      l = 35; h = 4.15; d = 5; r = 0.01;
      color("darkgreen", 0.8) translate([1,1,0.85]) rotate([-90,0,0]) hull() {
          translate([r, -h+r, 0]) cylinder(r=r,h=d);
          translate([r, -r, 0]) cylinder(r=r,h=d);
          translate([l-h/2, -h/2, 0]) cylinder(d=h,h=d);
      }

      // cable fixer
      translate([34,8.25,-1]) resize([2,5.6,30]) cylinder(d=6,h=30);
      translate([34,-0.25,-1]) resize([2,5.6,30]) cylinder(d=6,h=30);
      translate([30,0.5,-1]) resize([2,4.5,30]) cylinder(d=5,h=30);
      translate([24,6.75,-1]) resize([4.5,4.5,30]) cylinder(d=5,h=30);
    }
    
    // cable hole
      translate([35,4,3]) rotate([0,90,0]) cylinder(d=3.5,h=10);
    
    // pogopin
    pogo();
  }
  
  // cable simulator
  %hull() {
    translate([34,4,3]) sphere(d=2.5);
    translate([30,4.5,3]) sphere(d=3);
  }
  %hull() {
    translate([30,4.5,3]) sphere(d=3);
    translate([25,2.75,3]) sphere(d=3);
  }
  %hull() {
    translate([34,4,3]) sphere(d=2.5);
    translate([35,4,3]) rotate([0,90,0]) cylinder(d=2.5,h=10);
  }

}

module tip() {
  rotate([-90,0,0]) hull() {
    translate([-(length-height)/2,0,-depth]) cylinder(d=height,h=10);
    translate([(length-height)/2,0,-depth]) cylinder(d=height,h=10);
  }
}
