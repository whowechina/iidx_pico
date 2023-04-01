//main_button();
//aux_button();

module main_button() {
  frame(24.2, 41.2, 4.4, 1.8, 0.5, 3.3, 1.5, 0);
}

module aux_button() {
  frame(18, 18, 3.4, 1.5, 1.5, 3.5, 1.5, 0);
}

module frame(w, h, x1, x2, d1, d2, sm, lift)
{
  difference() {
    minkowski() {
      difference() {
        color("brown") hull() {
          framecube(w,h,d1,x1,-sm,lift);
          framecube(w,h,d2,x2,-sm,lift);
        }

        color("blue") {
          translate([0,0,-1]) framecube(w,h,d2+2,0,sm,lift);
        }
      }
      sphere(d=sm,$fn=48);
    }
    translate([0,0,-5]) cube([100,100,10],center=true);
  }
}

module framecube(w,h,d,x,sm,lift) {
  translate([0,0,d/2]) cube([w+x*2+sm,h+x*2+sm,d+lift*2],center=true);
}

