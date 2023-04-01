difference() {
  union() {
    cylinder(d=159,h=0.8,$fn=200);
    cylinder(d=158,h=6,$fn=200);
    translate([0,0,5.2]) cylinder(d=159,h=0.98,$fn=200);
  }
  translate([0,0,-1]) cylinder(d=152.6,h=20,$fn=200);
  translate([0,0,2]) cylinder(d=154.6,h=20,$fn=200);
  translate([0,-2,2]) cube([130,4,10]);
}

difference()
{
  union() {
    for (i=[30:60:360]) {
      rotate([0,0,i]) translate([0,-1.5,0]) cube([78,3,1.5]);
    }
  }
  translate([0,-3.1,2]) cube([100,4,10]);
  translate([0,0,-1]) cylinder(d=142,h=4,$fn=200);
}

