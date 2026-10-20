// transalp_pd06_case.scad
// =============================================================================
// TRANSALP Bordcomputer — parametric enclosure for Honda Transalp 600 (PD06)
//
// 3-part ASA/PETG case derived from docs/CASE-DESIGN.md:
//   1. base_shell()    — Unterteil (Basisschale): PCB guide rails, screw pillars,
//                        cable-gland knockouts (underside), ball-joint mount interface
//   2. lid()            — Deckel: display cutout, ePTFE vent, O-ring groove,
//                        shingled overlap seam on back/underside
//   3. display_frame()  — Display-Rahmen (PETG insert): fixes E-Paper module on
//                        silicone pad, screws to lid
//
// Outer dims (prototype): 100 x 80 x 45 mm (W x H x D)
// Material: ASA (outer shell, light colour), PETG (display frame)
//
// Usage:  open this file, choose a part via the PART variable (or render all),
//         export STL with File > Export > Export as STL.
//
// Render a single part:
//   PART = "base";     // base_shell()
//   PART = "lid";      // lid()
//   PART = "frame";    // display_frame()
//   PART = "all";      // all three laid out side by side
// =============================================================================

// --- Master parameters (CASE-DESIGN.md §1 / §6) ----------------------------
OUTER_W = 100;     // X width
OUTER_H = 80;      // Y height (front face)
OUTER_D = 45;      // Z depth
WALL_OUT = 3.2;    // outer wall thickness (8 layers @ 0.4)
WALL_LID = 3.2;   // lid thickness
WALL_BASE = 3.2;  // base thickness
PILLAR_OD = 6.0;  // screw pillar outer diameter
PILLAR_H = 22;    // pillar height (internal)
M3_CLEAR = 3.4;   // M3 threaded-insert bore (brass insert melted in)
CORNER_R = 4.0;   // outer corner radius

// PCB (PCB-LAYOUT.md: 80 x 50 mm) + clearance
PCB_W = 80;
PCB_H = 50;
PCB_PLAY = 0.4;    // slide tolerance in guide rails
RAIL_H = 3.0;      // guide rail height above floor
RAIL_W = 2.0;      // guide rail width

// E-Paper 2.9" module (CASE-DESIGN.md §4)
EPD_W = 89;
EPD_H = 38;
EPD_WIN_W = EPD_W + 2;   // window aperture + margin
EPD_WIN_H = EPD_H + 2;
WIN_THICK = 2.0;          // acrylic window thickness

// Cable glands (underside)
GLAND_OD = 12.5;   // PG7/M12 nominal thread Ø
GLAND_COUNT = 4;
GLAND_Y = 10;      // distance from base edge to gland centerline

// O-ring groove
ORING_CORD = 2.0;  // 2 mm silicone cord
ORING_GROOVE_D = ORING_CORD + 0.2;
ORING_GROOVE_W = ORING_CORD + 0.4;

// Ball-joint / crossbar mount (22 mm handlebar)
BAR_DIA = 22.0;
UBOLT_CLEAR = BAR_DIA + 2.0;
DAMPER_H = 10.0;   // rubber-metal damper height

// ePTFE pressure vent
VENT_D = 6.0;       // M6 vent

// USB-C panel mount
USBC_W = 12;
USBC_H = 9;

$fn = 48;

// --- Helper: rounded box -----------------------------------------------------
module rbox(x, y, z, r) {
    hull() for (px = [-1, 1], py = [-1, 1])
        translate([(px*(x/2 - r)), (py*(y/2 - r)), 0])
            cylinder(h = z, r = r);
}

// --- Screw pillar (internal, brass-insert ready) -----------------------------
module pillar(h) {
    difference() {
        cylinder(h = h, d = PILLAR_OD);
        translate([0, 0, -0.1])
            cylinder(h = h + 0.2, d = M3_CLEAR);
    }
}

// Pillar positions (4 corners of the PCB footprint, inboard)
function pillar_xy() = [
    [ PCB_W/2 - 5,  PCB_H/2 - 5],
    [-PCB_W/2 + 5,  PCB_H/2 - 5],
    [ PCB_W/2 - 5, -PCB_H/2 + 5],
    [-PCB_W/2 + 5, -PCB_H/2 + 5],
];

// --- PCB guide rails ---------------------------------------------------------
module guide_rails() {
    for (sy = [-1, 1]) {
        translate([-PCB_W/2 - PCB_PLAY, sy*(PCB_H/2 + PCB_PLAY) - RAIL_W/2, 0])
            cube([PCB_W + 2*PCB_PLAY, RAIL_W, RAIL_H]);
        translate([PCB_W/2 + PCB_PLAY - RAIL_W/2, sy*(PCB_H/2 + PCB_PLAY/2) - RAIL_W, 0])
            cube([RAIL_W, PCB_H + PCB_PLAY, RAIL_H]);
    }
}

// === Part 1: base shell (Unterteil) ==========================================
module base_shell() {
    inner_w = OUTER_W - 2*WALL_OUT;
    inner_h = OUTER_H - 2*WALL_OUT;
    inner_d = OUTER_D - WALL_BASE;

    difference() {
        // outer shell
        translate([0, 0, OUTER_D/2])
            rbox(OUTER_W, OUTER_H, OUTER_D, CORNER_R);

        // interior cavity (open top)
        translate([0, 0, WALL_BASE])
            rbox(inner_w, inner_h, inner_d + 1, CORNER_R - 1);

        // cable-gland knockouts (underside, evenly spaced)
        for (i = [0 : GLAND_COUNT - 1])
            translate([
                -inner_w/2 + (i + 0.5) * (inner_w / GLAND_COUNT),
                -OUTER_H/2 + GLAND_Y,
                0
            ])
                cylinder(h = WALL_BASE + 0.2, d = GLAND_OD);

        // ePTFE pressure vent (underside, vertically oriented hole)
        translate([inner_w/2 - 8, OUTER_H/2 - 8, 0])
            cylinder(h = WALL_BASE + 0.2, d = VENT_D);

        // USB-C panel-mount (underside, downward-facing)
        translate([-inner_w/2 + 12, -OUTER_H/2 + GLAND_Y, 0])
            translate([0, 18, 0])
                cube([USBC_W, USBC_H, WALL_BASE + 0.2], center = true);

        // ball-joint mount interface (back wall)
        translate([0, OUTER_H/2, OUTER_D/2])
            rotate([90, 0, 0])
                cylinder(h = WALL_OUT + 0.2, d = UBOLT_CLEAR);
    }

    // screw pillars
    for (p = pillar_xy())
        translate([p[0], p[1], WALL_BASE])
            pillar(PILLAR_H);

    // PCB guide rails
    translate([0, 0, WALL_BASE])
        guide_rails();

    // stiffening ribs (diagonal, 1.2 x 3 mm)
    rib_h = 3.0;
    rib_t = 1.2;
    for (a = [45, 135, 225, 315]) {
        translate([0, 0, WALL_BASE])
            rotate([0, 0, a])
                translate([-inner_w/2, -rib_t/2, 0])
                    cube([inner_w, rib_t, rib_h]);
    }

    // vibration damper seats on back (2x M6 rubber-metal)
    for (sx = [-1, 1])
        translate([sx * 20, OUTER_H/2 - WALL_OUT/2, OUTER_D/2])
            rotate([90, 0, 0])
                cylinder(h = 2, d = 12);
}

// === Part 2: lid (Deckel) ===================================================
module lid() {
    lid_d = WALL_LID;
    inner_w = OUTER_W - 2*WALL_OUT;
    inner_h = OUTER_H - 2*WALL_OUT;

    difference() {
        // lid slab (slightly larger to shingle over base)
        translate([0, 0, lid_d/2])
            rbox(OUTER_W, OUTER_H, lid_d, CORNER_R);

        // display window cutout (front face)
        cube([EPD_WIN_W, EPD_WIN_H, lid_d + 0.2], center = true);

        // ePTFE vent hole (underside of lid, vertical)
        translate([inner_w/2 - 8, -inner_h/2 + 8, 0])
            cylinder(h = lid_d + 0.2, d = VENT_D);

        // O-ring groove (perimeter, underside)
        difference() {
            translate([0, 0, lid_d - ORING_GROOVE_D/2])
                rbox(inner_w, inner_h, ORING_GROOVE_D, CORNER_R - 1);
            translate([0, 0, lid_d - ORING_GROOVE_D/2])
                rbox(inner_w - 2*ORING_GROOVE_W,
                     inner_h - 2*ORING_GROOVE_W, ORING_GROOVE_D + 0.2, CORNER_R - 2);
        }

        // lid screw holes (4x M3 countersunk)
        for (p = pillar_xy())
            translate([p[0], p[1], 0])
                cylinder(h = lid_d + 0.2, d = M3_CLEAR);

        // display-frame screw bosses (4x M2.5, around window)
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx*(EPD_WIN_W/2 + 3), sy*(EPD_WIN_H/2 + 3), 0])
                cylinder(h = lid_d + 0.2, d = 2.6);
    }
}

// === Part 3: display frame (PETG insert) =====================================
module display_frame() {
    frame_t = 2.0;
    difference() {
        // frame slab slightly smaller than window
        cube([EPD_WIN_W - 1, EPD_WIN_H - 1, frame_t], center = true);
        // module opening (module rests on silicone pad below)
        cube([EPD_W - 4, EPD_H - 4, frame_t + 0.2], center = true);
        // M2.5 mounting holes
        for (sx = [-1, 1], sy = [-1, 1])
            translate([sx*(EPD_WIN_W/2 + 2), sy*(EPD_WIN_H/2 + 2), 0])
                cylinder(h = frame_t + 0.2, d = 2.6);
    }
}

// --- Rendering switch --------------------------------------------------------
PART = "all";   // "base" | "lid" | "frame" | "all"

if (PART == "base" || PART == "all") {
    if (PART == "all") translate([0, 0, 0]);
    base_shell();
}
if (PART == "lid" || PART == "all") {
    if (PART == "all") translate([0, -OUTER_H - 10, 0]);
    lid();
}
if (PART == "frame" || PART == "all") {
    if (PART == "all") translate([0, -2*OUTER_H - 20, 0]);
    display_frame();
}
