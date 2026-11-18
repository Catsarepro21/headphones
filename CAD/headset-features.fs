FeatureScript 3112;
import(path : "onshape/std/common.fs", version : "3112.0");

const AMBIENT_PORT = vector(45 * cos(48 * degree), 55 * sin(48 * degree), 16.1);
const AMBIENT_DIRECTION = normalize(vector(AMBIENT_PORT[0] / (45 * 45), AMBIENT_PORT[1] / (55 * 55), 0));

const REACH = { (millimeter) : [65, 83, 120] } as LengthBoundSpec;
const CAPSULE = { (millimeter) : [8, 10.2, 13] } as LengthBoundSpec;
const SLOT = { (millimeter) : [19, 23, 24] } as LengthBoundSpec;
const SPAN = { (millimeter) : [220, 245, 280] } as LengthBoundSpec;

function prism(context is Context, id is Id, lo is Vector, hi is Vector) returns Query
{
    fCuboid(context, id, { "corner1" : lo * millimeter, "corner2" : hi * millimeter });
    return qCreatedBy(id, EntityType.BODY);
}

function cylinder(context is Context, id is Id, a is Vector, b is Vector, radius is number) returns Query
{
    fCylinder(context, id, { "bottomCenter" : a * millimeter, "topCenter" : b * millimeter,
                            "radius" : radius * millimeter });
    return qCreatedBy(id, EntityType.BODY);
}

function join(context is Context, id is Id, parts is array) returns Query
{
    const source = qUnion(parts);
    opBoolean(context, id, { "tools" : source, "operationType" : BooleanOperationType.UNION });
    return source;
}

function cut(context is Context, id is Id, target is Query, tools is array)
{
    opBoolean(context, id, { "targets" : target, "tools" : qUnion(tools),
                            "operationType" : BooleanOperationType.SUBTRACTION });
}

function finish(context is Context, body is Query, name is string, tint is Color)
{
    setProperty(context, { "entities" : body, "propertyType" : PropertyType.NAME, "value" : name });
    setProperty(context, { "entities" : body, "propertyType" : PropertyType.APPEARANCE, "value" : tint });
}

// A planar strip between two centers, inclined in the local XY plane.
function strip(context is Context, id is Id, a is Vector, b is Vector,
               halfWidth is number, base is number, thickness is number) returns Query
{
    const d = normalize(b - a);
    const n = halfWidth * vector(-d[1], d[0]);
    const corners = [a + n, a - n, b - n, b + n];
    const profile = newSketchOnPlane(context, id + "profile", {
        "sketchPlane" : plane(vector(0, 0, base) * millimeter, vector(0, 0, 1), vector(1, 0, 0)) });
    for (var i = 0; i < 4; i += 1)
        skLineSegment(profile, "side" ~ i, { "start" : corners[i] * millimeter,
                   "end" : corners[(i + 1) % 4] * millimeter });
    skSolve(profile);
    opExtrude(context, id + "solid", { "entities" : qSketchRegion(id + "profile"),
        "direction" : vector(0, 0, 1), "endBound" : BoundingType.BLIND,
        "endDepth" : thickness * millimeter });
    opDeleteBodies(context, id + "removeProfile", { "entities" : qCreatedBy(id + "profile", EntityType.BODY) });
    return qCreatedBy(id + "solid", EntityType.BODY);
}

// Local X is outward, local Y is along the slide and local Z is toward the cushion.
const POWER_ANGLE = 135 * degree;
const POWER_CENTER = vector(45 * cos(POWER_ANGLE), 55 * sin(POWER_ANGLE), 24.75);
const POWER_NORMAL = normalize(vector(POWER_CENTER[0] / (45 * 45), POWER_CENTER[1] / (55 * 55), 0));
const POWER_FRAME = coordSystem(POWER_CENTER * millimeter, POWER_NORMAL, vector(0, 0, 1));
const POWER_SEAT = -3.2;

function powerBox(context is Context, id is Id, lo is Vector, hi is Vector) returns Query
{
    const body = prism(context, id + "stock", lo, hi);
    opTransform(context, id + "orient", { "bodies" : body, "transform" : toWorld(POWER_FRAME) });
    return body;
}

function powerHole(context is Context, id is Id, inner is number, outer is number,
                   offset is number, radius is number) returns Query
{
    const body = cylinder(context, id + "stock", vector(inner, offset, 0), vector(outer, offset, 0), radius);
    opTransform(context, id + "orient", { "bodies" : body, "transform" : toWorld(POWER_FRAME) });
    return body;
}

function powerSwitchReference(context is Context, id is Id)
{
    // C&K 1101M1S3ZQE2. Datasheet body, M1 tabs, S3 cap and Z solder-lug envelope.
    const face = POWER_SEAT;
    var shapes = [powerBox(context, id + "case", vector(face - 6.35, -6.35, -3.3), vector(face, 6.35, 3.3)),
        powerBox(context, id + "tabs", vector(face - 0.65, -9.78, -3.495), vector(face, 9.78, 3.495)),
        powerBox(context, id + "slide", vector(face - 0.1, -3.135, -1.93), vector(face + 5.08, 0.725, 1.93))];
    for (var i = 0; i < 3; i += 1)
    {
        const y = (i - 1) * 4.70;
        shapes = append(shapes, powerBox(context, id + ("lug" ~ i), vector(face - 11.13, y - 0.64, -0.65),
            vector(face - 6.25, y + 0.64, 0.65)));
    }
    const power = join(context, id + "complete", shapes);
    cut(context, id + "drillTabs", power, [powerHole(context, id + "tabA", face - 1, face + 0.2, -8.765, 1.195),
        powerHole(context, id + "tabB", face - 1, face + 0.2, 8.765, 1.195)]);
    finish(context, power, "C&K 1101M1S3ZQE2 power switch - drawing reference", color(0.22, 0.23, 0.24));
}

function powerSwitchMount(context is Context, id is Id, shell is Query)
{
    const face = POWER_SEAT;
    var stock = [shell];
    var tools = [];
    for (var i = 0; i < 2; i += 1)
    {
        const key = id + ("mountingEar" ~ i);
        const y = (2 * i - 1) * 8.765;
        stock = append(stock, powerBox(context, key + "seat", vector(face, y - 2.6, -3.6),
            vector(-1.3, y + 2.6, 3.6)));
        tools = append(tools, powerHole(context, key + "m2Clearance", face - 0.3, 3, y, 1.1));
        tools = append(tools, powerHole(context, key + "headRelief", -1, 3, y, 1.98));
    }
    const housing = join(context, id + "integrateSeats", stock);
    tools = append(tools, powerBox(context, id + "solderAndHarness", vector(face - 13.3, -6.65, -2.1),
        vector(face - 6.25, 6.65, 2.1)));
    tools = append(tools, powerBox(context, id + "switchBodyClearance", vector(face - 6.7, -6.60, -3.52),
        vector(face + 0.02, 6.60, 3.52)));
    tools = append(tools, powerBox(context, id + "mountingTabClearance", vector(face - 0.85, -10.02, -3.70),
        vector(face + 0.02, 10.02, 3.70)));
    tools = append(tools, powerBox(context, id + "fullSlideTravel", vector(face - 0.2, -3.42, -2.12),
        vector(3.5, 3.42, 2.12)));
    cut(context, id + "switchAndFastenerAccess", housing, tools);
}

// Right selector: X is outward, Y is along the slider and Z is toward the ear pad.
const MODE_X = 25.2;
const MODE_Y = -55 * sqrt(1 - MODE_X * MODE_X / (45 * 45));
const MODE_N = normalize(vector(MODE_X / (45 * 45), MODE_Y / (55 * 55), 0));
// Leave manufacturing clearance to the installed four-button PCB behind the switch housing.
const MODE_STANDOFF = 0.65;
const MODE_FRAME = coordSystem((vector(MODE_X, MODE_Y, 0) + MODE_STANDOFF * MODE_N) * millimeter, MODE_N, vector(0, 0, 1));
const MODE_PANEL_HOLES = [-7.67, 7.67];
const MODE_BOARD_Z = 11.45;
const MODE_TRAVEL = 2.0;
const MODE_POSITION = -MODE_TRAVEL / 2;

function modePlace(context is Context, id is Id, body is Query) returns Query
{
    opTransform(context, id, { "bodies" : body, "transform" : toWorld(MODE_FRAME) });
    return body;
}

function modeBox(context is Context, id is Id, lo is Vector, hi is Vector) returns Query
{
    return modePlace(context, id + "orient", prism(context, id + "stock", lo, hi));
}

function modeBore(context is Context, id is Id, r0 is number, r1 is number,
                  y is number, z is number, radius is number) returns Query
{
    return modePlace(context, id + "orient", cylinder(context, id + "stock", vector(r0, y, z), vector(r1, y, z), radius));
}

function modePanel(context is Context, id is Id, r0 is number, r1 is number,
                   halfWidth is number, z0 is number, z1 is number, corner is number) returns Query
{
    const p = [vector(-halfWidth + corner, z0), vector(halfWidth - corner, z0),
        vector(halfWidth, z0 + corner), vector(halfWidth, z1 - corner),
        vector(halfWidth - corner, z1), vector(-halfWidth + corner, z1),
        vector(-halfWidth, z1 - corner), vector(-halfWidth, z0 + corner)];
    const d = corner / sqrt(2);
    const centers = [vector(halfWidth - corner + d, z0 + corner - d), vector(halfWidth - corner + d, z1 - corner + d),
        vector(-halfWidth + corner - d, z1 - corner + d), vector(-halfWidth + corner - d, z0 + corner - d)];
    const profile = newSketchOnPlane(context, id + "profile", {
        "sketchPlane" : plane(vector(r0, 0, 0) * millimeter, vector(1, 0, 0), vector(0, 1, 0)) });
    for (var i = 0; i < 4; i += 1)
    {
        skLineSegment(profile, "side" ~ i, { "start" : p[2 * i] * millimeter,
            "end" : p[2 * i + 1] * millimeter });
        skArc(profile, "corner" ~ i, { "start" : p[2 * i + 1] * millimeter,
            "mid" : centers[i] * millimeter, "end" : p[(2 * i + 2) % 8] * millimeter });
    }
    skSolve(profile);
    opExtrude(context, id + "solid", { "entities" : qSketchRegion(id + "profile"),
        "direction" : vector(1, 0, 0), "endBound" : BoundingType.BLIND, "endDepth" : (r1 - r0) * millimeter });
    opDeleteBodies(context, id + "removeProfile", { "entities" : qCreatedBy(id + "profile", EntityType.BODY) });
    return modePlace(context, id + "orient", qCreatedBy(id + "solid", EntityType.BODY));
}

function modeShellSeat(context is Context, id is Id, shell is Query)
{
    // Capture the exact 11 x 7.45 x 1.0 mm KiCad board. The rear channel carries its two soldered leads.
    const collar = modePanel(context, id + "collar", -3.55, 0.75, 10.6, 8.85, 17.85, 1.6);
    const railA = modeBox(context, id + "railA", vector(-9.5, -6.3, 8.9), vector(-2.4, -3.05, 10.45));
    const railB = modeBox(context, id + "railB", vector(-8.0, 3.05, 8.9), vector(-2.4, 6.3, 10.45));
    const seat = join(context, id + "shellSeat", [shell, collar, railA, railB]);
    var cuts = [modeBox(context, id + "pcbPocket", vector(-10.0, -5.72, 10.34), vector(1.1, 5.72, 11.63)),
        modeBox(context, id + "bodyPocket", vector(-5.82, -4.75, 11.40), vector(1.1, 4.75, 15.20)),
        modeBox(context, id + "harnessPath", vector(-13.0, -2.25, 9.12), vector(-7.5, 4.5, 11.30))];
    for (var i = 0; i < 2; i += 1)
        cuts = append(cuts, modeBore(context, id + ("insert" ~ i), -3.30, 0.81, MODE_PANEL_HOLES[i], 13.35, 1.60));
    cut(context, id + "fitSelector", seat, cuts);
}

function modePartReferences(context is Context, id is Id)
{
    const black = color(0.14, 0.15, 0.16);
    const y = MODE_POSITION;
    const cover = modePanel(context, id + "cover", 1.05, 2.97, 10.6, 8.85, 17.85, 1.6);
    var coverStock = [cover,
        modeBox(context, id + "boardStopA", vector(-2.07, 4.78, 10.45), vector(1.35, 5.68, 11.60)),
        modeBox(context, id + "boardStopB", vector(-2.07, -5.68, 10.45), vector(1.35, -4.78, 11.60))];
    var coverCuts = [modeBox(context, id + "sliderThrow", vector(0.45, -2.8, 12.66), vector(3.15, 2.8, 15.34))];
    var sealCuts = [modeBox(context, id + "stemSeal", vector(0.60, y - 1.40, 12.90), vector(1.20, y + 1.40, 15.10)),
        modeBox(context, id + "tabSealA", vector(0.60, -5.71, 10.35), vector(1.20, -4.74, 11.63)),
        modeBox(context, id + "tabSealB", vector(0.60, 4.74, 10.35), vector(1.20, 5.71, 11.63))];
    for (var i = 0; i < 2; i += 1)
    {
        const t = MODE_PANEL_HOLES[i];
        coverStock = append(coverStock, modeBore(context, id + ("compressionStop" ~ i), 0.75, 1.15, t, 13.35, 1.95));
        coverCuts = append(coverCuts, modeBore(context, id + ("m2" ~ i), 0.65, 3.10, t, 13.35, 1.10));
        sealCuts = append(sealCuts, modeBore(context, id + ("sealRelief" ~ i), 0.63, 1.20, t, 13.35, 2.20));
    }
    const plate = join(context, id + "selectorCover", coverStock);
    cut(context, id + "coverOpenings", plate, coverCuts);
    finish(context, plate, "Mode selector - removable retainer", black);
    const silicone = modePanel(context, id + "diaphragm", 0.75, 1.05, 10.4, 9.05, 17.65, 1.4);
    cut(context, id + "sealOpenings", silicone, sealCuts);
    finish(context, silicone, "Mode selector - flexible seal compressed 0.30 mm", color(0.23, 0.24, 0.24));

    const neck = modeBox(context, id + "buttonNeck", vector(-1.06, y - 1.39, 12.90), vector(3.68, y + 1.39, 15.10));
    const grip = modeBox(context, id + "thumbFace", vector(2.97, y - 1.75, 12.82), vector(4.05, y + 1.75, 15.18));
    const cap = join(context, id + "selectorCap", [neck, grip]);
    cut(context, id + "actuatorSocket", cap, [modeBox(context, id + "socket", vector(-1.20, y - 0.81, 13.18), vector(0.23, y + 0.81, 14.82))]);
    finish(context, cap, "Mode selector - external slide cap", color(0.31, 0.33, 0.34));

    // KiCad carrier outline, including rear corners, dowels and the two plated solder holes.
    const outline = [vector(-2.15, -5.5), vector(-9.25, -5.5), vector(-9.6, -5.15),
        vector(-9.6, 5.15), vector(-9.25, 5.5), vector(-2.15, 5.5)];
    const boardSketch = newSketchOnPlane(context, id + "pcbOutline", {
        "sketchPlane" : plane(vector(0, 0, MODE_BOARD_Z - 1) * millimeter, vector(0, 0, 1), vector(1, 0, 0)) });
    for (var i = 0; i < size(outline); i += 1)
        skLineSegment(boardSketch, "edge" ~ i, { "start" : outline[i] * millimeter,
            "end" : outline[(i + 1) % size(outline)] * millimeter });
    skSolve(boardSketch);
    opExtrude(context, id + "bareCarrier", { "entities" : qSketchRegion(id + "pcbOutline"),
        "direction" : vector(0, 0, 1), "endBound" : BoundingType.BLIND, "endDepth" : 1 * millimeter });
    opDeleteBodies(context, id + "removeOutline", { "entities" : qCreatedBy(id + "pcbOutline", EntityType.BODY) });
    const board = modePlace(context, id + "orientBoard", qCreatedBy(id + "bareCarrier", EntityType.BODY));
    var holes = [];
    for (var i = 0; i < 4; i += 1)
    {
        const x0 = i < 2 ? -3.6 : -8.57;
        const yy = [-3.4, 3.4, 1.25, 3.75][i];
        const r = i < 2 ? 0.45 : 0.325;
        const key = id + ("pcbDrill" ~ i);
        holes = append(holes, modePlace(context, key + "orient", cylinder(context, key, vector(x0, yy, 10.2), vector(x0, yy, 11.6), r)));
    }
    cut(context, id + "drillCarrier", board, holes);
    const shell = modeBox(context, id + "jsCase", vector(-5.4, -4.5, MODE_BOARD_Z - .03), vector(-1.8, 4.5, MODE_BOARD_Z + 3.5));
    const stem = modeBox(context, id + "jsActuator", vector(-1.9, y - .75, 13.25), vector(.20, y + .75, 14.75));
    const populated = join(context, id + "populatedCarrier", [board, shell, stem]);
    finish(context, populated, "Mode selector - C&K JS102011SAQN on 1 mm PCB", color(0.12, 0.22, 0.20));
}

// Flip-to-mute cam and serviceable mount for the low-current Aratas B3F-1000.
const MUTE_X = 51.8;
const MUTE_Y = -9.5;
const MUTE_Z = 21.35;
const MUTE_BOSS_Z = 27.35;

function muteBox(context is Context, id is Id, x0 is number, x1 is number,
                 near is number, far is number, z0 is number, z1 is number) returns Query
{
    return prism(context, id, vector(x0, MUTE_Y - far, z0), vector(x1, MUTE_Y - near, z1));
}

function muteCylinder(context is Context, id is Id, x is number, z is number,
                      near is number, far is number, radius is number) returns Query
{
    return cylinder(context, id, vector(x, MUTE_Y - far, z), vector(x, MUTE_Y - near, z), radius);
}

function muteCamRadius(a is number) returns number
{
    if (a >= -160 && a <= -84)
        return 6.15;
    if (a < -160)
        return 5.14 + 1.01 * (a + 177) / 17;
    return 6.15 - 1.01 * (a + 84) / 13;
}

function muteCam(context is Context, id is Id) returns Query
{
    // Positive rotation about the boom axis raises the boom. The stationary follower sits at -90 degrees.
    const angles = [-177, -170, -165, -160, -155, -150, -145, -140, -135, -130, -125,
        -120, -115, -110, -105, -100, -95, -90, -85, -84, -80, -75, -71];
    var outline = [];
    for (var a in angles)
        outline = append(outline, vector(MUTE_X, MUTE_Y) + muteCamRadius(a) * vector(cos(a * degree), sin(a * degree)));
    for (var i = size(angles) - 1; i >= 0; i -= 1)
        outline = append(outline, vector(MUTE_X, MUTE_Y) + 4.55 * vector(cos(angles[i] * degree), sin(angles[i] * degree)));
    const profile = newSketchOnPlane(context, id + "profile", {
        "sketchPlane" : plane(vector(0, 0, 19.30) * millimeter, vector(0, 0, 1), vector(1, 0, 0)) });
    for (var i = 0; i < size(outline); i += 1)
        skLineSegment(profile, "edge" ~ i, { "start" : outline[i] * millimeter,
            "end" : outline[(i + 1) % size(outline)] * millimeter });
    skSolve(profile);
    opExtrude(context, id + "lobe", { "entities" : qSketchRegion(id + "profile"),
        "direction" : vector(0, 0, 1), "endBound" : BoundingType.BLIND, "endDepth" : 3.14 * millimeter });
    opDeleteBodies(context, id + "removeProfile", { "entities" : qCreatedBy(id + "profile", EntityType.BODY) });
    return qCreatedBy(id + "lobe", EntityType.BODY);
}

function muteSwitchSeat(context is Context, id is Id, shell is Query)
{
    const rear = muteBox(context, id + "body", 44.2, 58.2, 6.25, 15.60, 16.4, 26.1);
    const guide = muteCylinder(context, id + "followerGuide", MUTE_X, MUTE_Z, 6.25, 8.45, 2.9);
    const screwBoss = muteCylinder(context, id + "heatSetBoss", MUTE_X, MUTE_BOSS_Z, 11.20, 15.60, 2.96);
    const fitShell = join(context, id + "shellAddition", [shell, rear, guide, screwBoss]);
    const controls = muteBox(context, id + "switchCavity", 47.42, 56.17, 7.40, 15.80, 18.07, 24.65);
    const follower = muteCylinder(context, id + "followerBore", MUTE_X, MUTE_Z, 6.12, 8.45, 1.90);
    const insert = muteCylinder(context, id + "heatSetPilot", MUTE_X, MUTE_BOSS_Z, 11.55, 15.70, 1.60);
    const tongue = muteBox(context, id + "latchSlot", 48.70, 54.90, 14.08, 15.72, 16.50, 17.85);
    const cable = muteBox(context, id + "cablePath", 41.0, 49.0, 12.45, 14.45, 18.95, 20.75);
    cut(context, id + "switchOpenings", fitShell, [controls, follower, insert, tongue, cable]);
}

function muteSwitchParts(context is Context, id is Id)
{
    const panel = muteBox(context, id + "guard", 44.25, 58.15, 15.62, 17.52, 16.45, 26.05);
    const ear = muteCylinder(context, id + "screwEar", MUTE_X, MUTE_BOSS_Z, 15.62, 17.52, 2.96);
    const key = muteBox(context, id + "guideKey", 48.85, 54.75, 14.20, 15.93, 16.62, 17.65);
    const cover = join(context, id + "retainer", [panel, ear, key]);
    cut(context, id + "screwClearance", cover,
        [muteCylinder(context, id + "screwBore", MUTE_X, MUTE_BOSS_Z, 15.44, 17.70, 1.10)]);
    finish(context, cover, "Boom mute - removable guard", color(0.14, 0.15, 0.16));

    const follower = muteCylinder(context, id + "follower", MUTE_X, MUTE_Z, 6.18, 7.32, 1.68);
    finish(context, follower, "Boom mute - guided follower", color(0.32, 0.34, 0.35));

    const body = muteBox(context, id + "switchBody", MUTE_X - 3.0, MUTE_X + 3.0, 7.45, 10.85, MUTE_Z - 3.0, MUTE_Z + 3.0);
    const button = muteCylinder(context, id + "pressedButton", MUTE_X, MUTE_Z, 7.33, 7.52, 1.75);
    var stock = [body, button];
    for (var i = 0; i < 4; i += 1)
    {
        const x = MUTE_X + (i < 2 ? -3.25 : 3.25);
        const z = MUTE_Z + (i % 2 == 0 ? -2.25 : 2.25);
        stock = append(stock, muteBox(context, id + ("terminal" ~ i), x - 0.38, x + 0.38,
            10.65, 14.35, z - 0.17, z + 0.17));
    }
    const part = join(context, id + "switchReference", stock);
    finish(context, part, "Boom mute - Aratas B3F-1000 deployed reference", color(0.28, 0.27, 0.25));
}

annotation { "Feature Type Name" : "Headset boom and mounts" }
export const headsetMounts = defineFeature(function(context is Context, id is Id, definition is map)
    precondition
    {
        annotation { "Name" : "Pivot-to-capsule reach" }
        isLength(definition.reach, REACH);
        annotation { "Name" : "Microphone pocket diameter" }
        isLength(definition.capsule, CAPSULE);
        annotation { "Name" : "Provisional yoke slot width" }
        isLength(definition.yokeWidth, SLOT);
        annotation { "Name" : "Cup rear-to-rear span" }
        isLength(definition.span, SPAN);
    }
    {
        const black = color(0.14, 0.15, 0.17);
        const mount = color(0.27, 0.31, 0.35);
        const span = definition.span / millimeter;
        const reach = definition.reach / millimeter;

        // Feature coordinates match the headset assembly datum. The revolute mate locates the actual pivot.
        const pivot = vector(51.8, -9.5);
        const tip = pivot + vector(0.9701, -0.2425) * reach;
        const axisStart = vector(pivot[0], pivot[1], 18.35);
        const hinge = cylinder(context, id + "hinge", axisStart, vector(pivot[0], pivot[1], 22.70), 5.3);
        const micPod = cylinder(context, id + "pod", vector(tip[0], tip[1], 18.35), vector(tip[0], tip[1], 31.35), 7.1);
        const arm = strip(context, id + "shaft", pivot, tip, 3.45, 18.35, 4.35);
        const cam = muteCam(context, id + "muteCam");
        const boom = join(context, id + "uniteBoom", [hinge, arm, micPod, cam]);
        const pin = cylinder(context, id + "pinBore", vector(pivot[0], pivot[1], 17.8), vector(pivot[0], pivot[1], 23.2), 1.6);
        const pocket = cylinder(context, id + "capsulePocket", vector(tip[0], tip[1], 20.2), vector(tip[0], tip[1], 31.8), definition.capsule / (2 * millimeter));
        const race = strip(context, id + "wireRace", pivot + vector(4, -1), tip, 1.25, 21.15, 10.45);
        cut(context, id + "detailBoom", boom, [pin, pocket, race]);
        finish(context, boom, "Fold-up boom - PUI 9.7 mm capsule pocket", black);
        opMateConnector(context, id + "boomAxis", {
            "coordSystem" : coordSystem(axisStart * millimeter, vector(1, 0, 0), vector(0, 0, 1)), "owner" : boom });


        // Nominal PUI drawing envelope: 9.7 mm can and 4.5 mm body. Pocket reserves 5 mm behind the can for the terminals.
        const microphone = cylinder(context, id + "puiCapsule", vector(tip[0], tip[1], 26.65), vector(tip[0], tip[1], 31.15), 4.85);
        finish(context, microphone, "PUI AOM-4544P-2-R microphone capsule", color(0.20, 0.21, 0.22));


        // Captive plungers install from the inside before the switch plate.
        const buttonX = [-25.0091, -15.8104, 16.1936, 25.20075];
        const buttonNames = ["Play pause plunger", "Volume down plunger", "Volume up plunger", "Pairing plunger"];
        for (var i = 0; i < size(buttonX); i += 1)
        {
            const buttonId = id + ("plunger" ~ i);
            const x = buttonX[i];
            const tipY = -55 * sqrt(1 - x * x / (45 * 45)) - 1.15;
            const stem = cylinder(context, buttonId + "stem", vector(x, tipY, 22.9969), vector(x, -40.57, 22.9969), 1.54);
            const flange = cylinder(context, buttonId + "flange", vector(x, -41.20, 22.9969), vector(x, -40.38, 22.9969), 2.19);
            const plunger = join(context, buttonId + "join", [stem, flange]);
            finish(context, plunger, buttonNames[i], black);
        }


        const ambient = cylinder(context, id + "ambientCapsule", AMBIENT_PORT - 8.05 * AMBIENT_DIRECTION,
            AMBIENT_PORT - 3.55 * AMBIENT_DIRECTION, 4.85);
        finish(context, ambient, "Right ambient PUI AOM-4544P-2-R capsule", color(0.29, 0.30, 0.31));

        powerSwitchReference(context, id + "powerSwitch");
        modePartReferences(context, id + "modeSelector");
        muteSwitchParts(context, id + "boomMute");

        // Two sleeves, with adjustment slots in their cheeks. The vendor yoke thickness and pin spacing are provisional.
        const bw = definition.yokeWidth / (2 * millimeter);
        for (var side in [0, 1])
        {
            const prefix = id + ("side" ~ side);
            const z = side == 0 ? 16.35 : span - 16.35;
            const sleeve = prism(context, prefix + "adapter", vector(-14, 55.15, z - 6.75), vector(14, 72.7, z + 6.75));
            const bandSlot = prism(context, prefix + "bandSlot", vector(-bw, 54.9, z - 4.85), vector(bw, 73, z + 4.85));
            const adjust = prism(context, prefix + "slideSlot", vector(-15, 59.2, z - 1.75), vector(15, 69.2, z + 1.75));
            const route = prism(context, prefix + "wireSlot", vector(7.1, 54.9, z + 4.6), vector(10.7, 73, z + 8));
            cut(context, prefix + "openAdapter", sleeve, [bandSlot, adjust, route]);
            // Lead the inter-cup harness beside the adjustment slot.
            const guide = cylinder(context, prefix + "harnessGuide", vector(17.2, 55.45, z),
                vector(17.2, 58.55, z), 5.25);
            const guidedSleeve = join(context, prefix + "combineHarnessGuide", [sleeve, guide]);
            const guideBore = cylinder(context, prefix + "harnessGuideBore", vector(17.2, 55.30, z),
                vector(17.2, 58.70, z), 3.42);
            cut(context, prefix + "openHarnessGuide", guidedSleeve, [guideBore]);

            // The slit lets the grommet be fitted around a terminated harness.
            const grommetTube = cylinder(context, prefix + "grommetTube", vector(17.2, 45.85, z),
                vector(17.2, 53.13, z), 3.77);
            const grommetLip = cylinder(context, prefix + "grommetLip", vector(17.2, 53.075, z),
                vector(17.2, 54.08, z), 4.60);
            const grommet = join(context, prefix + "joinGrommet", [grommetTube, grommetLip]);
            const grommetBore = cylinder(context, prefix + "grommetBore", vector(17.2, 45.70, z),
                vector(17.2, 54.21, z), 3.21);
            const slit = prism(context, prefix + "grommetSlit", vector(17.10, 45.60, z + 0.10),
                vector(17.35, 54.22, z + 4.85));
            cut(context, prefix + "openGrommet", grommet, [grommetBore, slit]);
            finish(context, grommet, side == 0 ? "Right headband cable grommet - TPU" :
                "Left headband cable grommet - TPU", color(0.18, 0.18, 0.19));

            finish(context, sleeve, side == 0 ? "Right slotted yoke adapter - provisional" : "Left slotted yoke adapter - provisional", mount);


            // The vendor's selected 110 mm variant labels the distance between the inward fork tips.
            // Thickness, pin diameter and side offsets are fit allowances pending caliper measurements.
            const sideName = side == 0 ? "Right " : "Left ";
            const forkZ = side == 0 ? 27.2 : span - 27.2;
            const fork = cylinder(context, prefix + "steelFork", vector(0, 0, forkZ - 4), vector(0, 0, forkZ + 4), 57.45);
            const forkClear = cylinder(context, prefix + "jawSpan110", vector(0, 0, forkZ - 4.3), vector(0, 0, forkZ + 4.3), 55);
            const clip = prism(context, prefix + "trimFork", vector(-61, -61, forkZ - 5), vector(61, -3.6, forkZ + 5));
            const pinPassage = cylinder(context, prefix + "yokeAxle", vector(-60, 0, forkZ), vector(60, 0, forkZ), 1.55);
            cut(context, prefix + "formYoke", fork, [forkClear, clip, pinPassage]);
            finish(context, fork, sideName ~ "OKCSC 110 mm metal fork - vendor photo reference", color(0.62, 0.65, 0.68));

            // The saddle contact is trimmed to the cup's 90 x 110 outer ellipse.
            for (var handed in [-1, 1])
            {
                const earId = prefix + ("saddle" ~ (handed + 1));
                const saddle = cylinder(context, earId + "stock", vector(handed * 43.9, 0, forkZ), vector(handed * 54.75, 0, forkZ), 3.85);
                const edgeMatch = newSketchOnPlane(context, earId + "contactProfile", {
                    "sketchPlane" : plane(vector(0, 0, forkZ - 5) * millimeter, vector(0, 0, 1)) });
                skEllipse(edgeMatch, "shell", { "center" : vector(0, 0) * millimeter, "majorRadius" : 55 * millimeter,
                    "minorRadius" : 45 * millimeter, "majorAxis" : vector(0, 1) });
                skSolve(edgeMatch);
                opExtrude(context, earId + "cupTool", { "entities" : qSketchRegion(earId + "contactProfile"),
                    "direction" : vector(0, 0, 1), "endBound" : BoundingType.BLIND, "endDepth" : 10 * millimeter });
                const bushing = cylinder(context, earId + "bore", vector(handed * 48.4, 0, forkZ), vector(handed * 56, 0, forkZ), 1.55);
                cut(context, earId + "shapeSaddle", saddle, [qCreatedBy(earId + "cupTool", EntityType.BODY), bushing]);
                opDeleteBodies(context, earId + "removeProfile", { "entities" : qCreatedBy(earId + "contactProfile", EntityType.BODY) });
                finish(context, saddle, sideName ~ (handed < 0 ? "rear" : "front") ~ " fork pivot saddle - verify pin", mount);
            }

            // The ring seat touches the bare PCB outside its 30 mm LED pitch radius.
            const a = side == 0 ? 2.8 : span - 8;
            const b = side == 0 ? 8 : span - 2.8;
            const seat = cylinder(context, prefix + "ringSeat", vector(0, 0, a), vector(0, 0, b), 39.2);
            const light = cylinder(context, prefix + "clearOptics", vector(0, 0, a - 0.2), vector(0, 0, b + 0.2), 34.8);
            cut(context, prefix + "openRingSeat", seat, [light]);
            finish(context, seat, side == 0 ? "Right RGB ring support collar" : "Left RGB ring support collar", mount);

            // Conservative stepped reference from the CE38MB-32 drawing, excluding terminals.
            const flangeStart = side == 0 ? 37 : span - 38.3;
            const flangeEnd = side == 0 ? 38.3 : span - 37;
            const basketStart = side == 0 ? 32.7 : span - 37.05;
            const basketEnd = side == 0 ? 37.05 : span - 32.7;
            const flange = cylinder(context, prefix + "daytonFlange", vector(0, 0, flangeStart), vector(0, 0, flangeEnd), 20);
            const basket = cylinder(context, prefix + "daytonBasket", vector(0, 0, basketStart), vector(0, 0, basketEnd), 17.75);
            const driver = join(context, prefix + "assembleDriver", [flange, basket]);
            finish(context, driver, side == 0 ? "Right Dayton CE38MB-32 envelope" : "Left Dayton CE38MB-32 envelope", black);
        }


        // Leather suspension strap traced from the BOM vendor photo. Profile and width are provisional.
        const strapSketch = newSketchOnPlane(context, id + "strapProfile", {
            "sketchPlane" : plane(vector(-19, 0, 0) * millimeter, vector(1, 0, 0), vector(0, 1, 0)) });
        const zCenter = span / 2;
        const ends = [vector(70, 21) * millimeter, vector(70, span - 21) * millimeter];
        const innerEnds = [vector(68.5, 23.1) * millimeter, vector(68.5, span - 23.1) * millimeter];
        skArc(strapSketch, "outer", { "start" : ends[0], "mid" : vector(144, zCenter) * millimeter, "end" : ends[1] });
        skArc(strapSketch, "inner", { "start" : innerEnds[1], "mid" : vector(141.3, zCenter) * millimeter, "end" : innerEnds[0] });
        skLineSegment(strapSketch, "rightEnd", { "start" : ends[0], "end" : innerEnds[0] });
        skLineSegment(strapSketch, "leftEnd", { "start" : innerEnds[1], "end" : ends[1] });
        skSolve(strapSketch);
        opExtrude(context, id + "strapSolid", { "entities" : qSketchRegion(id + "strapProfile"),
            "direction" : vector(1, 0, 0), "endBound" : BoundingType.BLIND, "endDepth" : 38 * millimeter });
        const strap = qCreatedBy(id + "strapSolid", EntityType.BODY);
        var perforations = [];
        for (var row in [-1, 1])
        {
            for (var station in [-3, -2, -1, 0, 1, 2, 3])
            {
                const z = zCenter + station * 15 + row * 3.5;
                perforations = append(perforations, cylinder(context, id + ("strapHole" ~ (row + 1) ~ "p" ~ (station + 3)),
                    vector(row * 7.5, 120, z), vector(row * 7.5, 148, z), 1.75));
            }
        }
        cut(context, id + "perforateStrap", strap, perforations);
        const clampClearance = prism(context, id + "strapEndTool", vector(-20, 64, 19), vector(20, 73, span - 19));
        cut(context, id + "clearClamps", strap, [clampClearance]);
        opDeleteBodies(context, id + "removeStrapSketch", { "entities" : qCreatedBy(id + "strapProfile", EntityType.BODY) });
        finish(context, strap, "OKCSC tan leather suspension strap - vendor photo reference", color(0.56, 0.26, 0.095));

        // The PCB rests on the four corner ledges. The rear and lower-left regions remain
        // open for the back-mounted BM83, its antenna, and through-hole header tails.
        const carrierBridge = prism(context, id + "mainCarrier", vector(-27.5, 28.4, 16.8), vector(27.5, 30.5, 19));
        var carrierPieces = [carrierBridge];
        for (var hand in [-1, 1])
        {
            const railId = id + ("mainRail" ~ (hand + 1));
            const outerSide = hand > 0 ? vector(32.25, -24, 16.8) : vector(-34.65, -24, 16.8);
            const innerSide = hand > 0 ? vector(34.65, 25.5, 19) : vector(-32.25, 25.5, 19);
            const sideRail = prism(context, railId + "rail", outerSide, innerSide);
            const rearLink = strip(context, railId + "rearLink", vector(hand * 33.2, 23.8), vector(hand * 26.9, 29.1), 1.4, 16.8, 2.2);
            const base = hand > 0 ? vector(39.5, -5.5, 2.8) : vector(-41.4, -5.5, 2.8);
            const end = hand > 0 ? vector(41.4, 5.5, 18.5) : vector(-39.5, 5.5, 18.5);
            const stand = prism(context, railId + "stand", base, end);
            const crossbar = strip(context, railId + "standLink", vector(hand * 33.9, 0), vector(hand * 40.4, 0), 2.7, 16.8, 2.2);
            for (var piece in [sideRail, rearLink, stand, crossbar])
                carrierPieces = append(carrierPieces, piece);
            for (var endSign in [-1, 1])
            {
                const legId = railId + ("end" ~ (endSign + 1));
                const y = endSign * 21.35;
                const lowX = hand > 0 ? 31.05 : -34.1;
                const highX = hand > 0 ? 34.1 : -31.05;
                const seat = prism(context, legId + "seat", vector(lowX, y - 1.8, 17.65), vector(highX, y + 1.8, 19));
                const wall = prism(context, legId + "upright", vector(outerSide[0], y - 1.8, 18.2), vector(innerSide[0], y + 1.8, 22.25));
                const clipLip = prism(context, legId + "catch", vector(lowX, y - 1.8, 21.10), vector(highX, y + 1.8, 22.25));
                for (var piece in [seat, wall, clipLip])
                    carrierPieces = append(carrierPieces, piece);
            }
        }
        const pcbRack = join(context, id + "fitMainCarrier", carrierPieces);
        const rightCapRelief = prism(context, id + "rightCapRelief", vector(-4.1, 27.8, 16.5), vector(4.1, 31.1, 18.0));
        cut(context, id + "clearRightRingCap", pcbRack, [rightCapRelief]);
        finish(context, pcbRack, "Right main PCB carrier - relieved for populated board", mount);

        const tray = prism(context, id + "batteryCarrier", vector(-24.5, -32, span - 18.8), vector(24.5, 32, span - 16.8));
        const padWindow = prism(context, id + "paddingWindow", vector(-18.5, -25, span - 19.2), vector(18.5, 25, span - 16.5));
        const slotLeft = prism(context, id + "leftStrapSlot", vector(-22.8, -8, span - 19.2), vector(-20.1, 8, span - 16.5));
        const slotRight = prism(context, id + "rightStrapSlot", vector(20.1, -8, span - 19.2), vector(22.8, 8, span - 16.5));
        const ringCapRelief = prism(context, id + "ringCapRelief", vector(-4.4, -32.5, span - 19.1), vector(4.4, -24.3, span - 16.5));
        // Support the three-position 2 mm Molex inline plug next to the cell.
        // Its latch faces the removable baffle. The battery half releases toward negative Y.
        const connectorShelf = prism(context, id + "batteryConnectorShelf",
            vector(23.55, -2.60, span - 18.62), vector(38.40, 16.10, span - 16.80));
        const shelfTieInboard = prism(context, id + "connectorTieInboard",
            vector(23.78, 9.60, span - 19.20), vector(24.80, 13.20, span - 16.60));
        const shelfTieOutboard = prism(context, id + "connectorTieOutboard",
            vector(36.67, 9.60, span - 19.20), vector(37.87, 13.20, span - 16.60));
        const serviceTray = join(context, id + "addBatteryConnectorShelf", [tray, connectorShelf]);
        cut(context, id + "openBatteryCarrier", serviceTray,
            [padWindow, slotLeft, slotRight, ringCapRelief, shelfTieInboard, shelfTieOutboard]);

        // Drawing-based maximum envelope, including the molded lock.
        const batteryConnector = prism(context, id + "batteryConnectorEnvelope",
            vector(24.95, -1.95, span - 27.80), vector(36.45, 16.95, span - 18.80));
        finish(context, batteryConnector, "Left Molex Micro-Lock Plus 3-pin mating envelope",
            color(0.22, 0.22, 0.24));
        finish(context, tray, "Left padded battery strap carrier - provisional", mount);
    });

const ACCESS_WIDTH = { (millimeter) : [13, 14.5, 18] } as LengthBoundSpec;
const ACCESS_HEIGHT = { (millimeter) : [7, 8, 10] } as LengthBoundSpec;

function harnessRectangle(context is Context, id is Id, hand is number, lo is Vector, hi is Vector) returns Query
{
    return prism(context, id, vector(min(hand * lo[0], hand * hi[0]), lo[1], lo[2]),
        vector(max(hand * lo[0], hand * hi[0]), hi[1], hi[2]));
}

function harnessCupPort(context is Context, id is Id, shell is Query, hand is number)
{
    const x = 17.2 * hand;
    const seat = cylinder(context, id + "seat", vector(x, 45.75, 16.35), vector(x, 53.05, 16.35), 5.40);
    const base = harnessRectangle(context, id + "tieBase", hand, vector(11.25, 42.2, 10.95), vector(23.15, 47.6, 13.65));
    const legA = harnessRectangle(context, id + "tieLegA", hand, vector(11.25, 43.5, 12.2), vector(13.25, 49.6, 14.2));
    const legB = harnessRectangle(context, id + "tieLegB", hand, vector(21.15, 43.5, 12.2), vector(23.15, 49.6, 14.2));
    const web = harnessRectangle(context, id + "tieWeb", hand, vector(13.25, 45.2, 12.3), vector(21.15, 47.8, 14.0));
    const reinforced = join(context, id + "joinToShell", [shell, seat, base, legA, legB, web]);
    const seatBore = cylinder(context, id + "seatBore", vector(x, 44.0, 16.35), vector(x, 54.4, 16.35), 3.85);
    const tieTunnel = harnessRectangle(context, id + "tieTunnel", hand, vector(15.5, 42.0, 10.35), vector(18.9, 48.25, 12.35));
    cut(context, id + "boreAndTunnel", reinforced, [seatBore, tieTunnel]);
}

annotation { "Feature Type Name" : "Right USB-C access and board cradle" }
export const fitUsb = defineFeature(function(context is Context, id is Id, definition is map)
    precondition
    {
        annotation { "Name" : "Configured cup shell", "Filter" : EntityType.BODY && BodyType.SOLID, "MaxNumberOfPicks" : 1 }
        definition.shell is Query;
        annotation { "Name" : "Cable overmold access width" }
        isLength(definition.accessWidth, ACCESS_WIDTH);
        annotation { "Name" : "Cable overmold access height" }
        isLength(definition.accessHeight, ACCESS_HEIGHT);
    }
    {
        const envelope = evBox3d(context, { "topology" : definition.shell, "tight" : true });
        harnessCupPort(context, id + "harnessPort", definition.shell, envelope.maxCorner[0] < 55 * millimeter ? -1 : 1);
        // Only the right shell includes the outboard boom boss beyond X = 55 mm.
        if (envelope.maxCorner[0] < 55 * millimeter)
            return;

        const halfWidth = definition.accessWidth / (2 * millimeter);
        const halfHeight = definition.accessHeight / (2 * millimeter);
        const axisZ = 22.175;
        const opening = prism(context, id + "plugAccess", vector(-halfWidth, -58, axisZ - halfHeight),
            vector(halfWidth, -48.8, axisZ + halfHeight));
        cut(context, id + "openPlugAccess", definition.shell, [opening]);

        var support = [definition.shell];
        for (var hand in [-1, 1])
        {
            const railId = id + ("cradleRail" ~ (hand + 1));
            const outerX = hand > 0 ? 13.7 : -13.7;
            const seatX = hand > 0 ? 11.05 : -11.05;
            const rimX = hand > 0 ? 12.5 : -12.5;
            const shoe = prism(context, railId + "shoe", vector(min(outerX, seatX), -51.4, 16.0),
                vector(max(outerX, seatX), -37.6, 19.0));
            const rim = prism(context, railId + "rim", vector(min(outerX, rimX), -50.4, 18),
                vector(max(outerX, rimX), -38.0, 22.5));
            const upperRail = prism(context, railId + "upperRail", vector(min(outerX, rimX), -40.0, 19.45),
                vector(max(outerX, rimX), -27.10, 22.5));
            const clip = prism(context, railId + "clip", vector(min(outerX, seatX), -43.2, 21.12),
                vector(max(outerX, seatX), -34.0, 22.5));
            const pushStop = prism(context, railId + "pushStop", vector(min(outerX, seatX), -27.46, 19.45),
                vector(max(outerX, seatX), -27.10, 22.15));
            const pullStop = prism(context, railId + "pullStop", vector(min(outerX, seatX), -50.9, 18.3),
                vector(max(outerX, seatX), -49.78, 22.15));
            for (var body in [shoe, rim, upperRail, clip, pushStop, pullStop])
                support = append(support, body);
        }
        join(context, id + "integrateCradle", support);
    });

annotation { "Feature Type Name" : "Right four-button board fit and guides" }
export const fitControls = defineFeature(function(context is Context, id is Id, definition is map)
    precondition
    {
        annotation { "Name" : "Configured cup shell", "Filter" : EntityType.BODY && BodyType.SOLID, "MaxNumberOfPicks" : 1 }
        definition.shell is Query;
    }
    {
        const envelope = evBox3d(context, { "topology" : definition.shell, "tight" : true });
        if (envelope.maxCorner[0] < 55 * millimeter)
            return;

        // Board layout: headset-controls.kicad_pcb. STEP origin 60,60 with X-axis rotated 90 deg.
        // The back datum is Y=-37.05 mm and the switch travel direction is +Y.
        const plateZ = 22.9969;
        const holeCenters = [-25.0091, -15.8104, 16.1936, 25.20075];
        const switchCenters = [-25.009, -17.05, 17.05, 25.201];
        var reliefs = [];
        var guides = [definition.shell];
        const bridge = prism(context, id + "bridge", vector(-28.45, -39.10, 11.5), vector(28.45, -36.86, 15.75));
        reliefs = append(reliefs, bridge);
        for (var hand in [-1, 1])
        {
            const key = id + ("wing" ~ (hand + 1));
            const boardEdge = prism(context, key + "boardEdge", vector(min(hand * 14.05, hand * 28.45), -39.10, 15.55),
                vector(max(hand * 14.05, hand * 28.45), -36.86, 28.4));
            reliefs = append(reliefs, boardEdge);
            const wall = prism(context, key + "wall", vector(min(hand * 28.36, hand * 30.05), -40.5, 11.47),
                vector(max(hand * 28.36, hand * 30.05), -36.56, 19.0));
            const frontLip = prism(context, key + "frontLip", vector(min(hand * 27.7, hand * 29.2), -39.42, 13.1),
                vector(max(hand * 27.7, hand * 29.2), -38.86, 17.6));
            const backLip = prism(context, key + "backLip", vector(min(hand * 27.7, hand * 29.2), -36.82, 13.1),
                vector(max(hand * 27.7, hand * 29.2), -36.15, 17.6));
            const base = prism(context, key + "shelf", vector(min(hand * 24.4, hand * 29.05), -39.44, 10.7),
                vector(max(hand * 24.4, hand * 29.05), -36.56, 11.62));
            for (var body in [wall, frontLip, backLip, base])
                guides = append(guides, body);
        }
        for (var i = 0; i < size(holeCenters); i += 1)
        {
            const key = id + ("button" ~ i);
            const boreX = holeCenters[i];
            const switchX = switchCenters[i];
            const switchRelief = prism(context, key + "switchRelief", vector(switchX - 2.86, -40.65, 19.5),
                vector(switchX + 2.86, -38.32, 26.55));
            const buttonRelief = cylinder(context, key + "shoulderRelief", vector(boreX, -42.15, plateZ),
                vector(boreX, -38.2, plateZ), 2.42);
            reliefs = append(reliefs, switchRelief);
            reliefs = append(reliefs, buttonRelief);
        }
        const cup = join(context, id + "addPlateGuides", guides);
        cut(context, id + "fitPlateAndSwitches", cup, reliefs);
        powerSwitchMount(context, id + "masterPower", cup);
        modeShellSeat(context, id + "modeSelector", cup);
        muteSwitchSeat(context, id + "boomMute", cup);
    });

annotation { "Feature Type Name" : "Right ambient mic seat and acoustic port" }
export const ambientSeat = defineFeature(function(context is Context, id is Id, definition is map)
    precondition
    {
        annotation { "Name" : "Configured cup shell", "Filter" : EntityType.BODY && BodyType.SOLID, "MaxNumberOfPicks" : 1 }
        definition.shell is Query;
    }
    {
        const envelope = evBox3d(context, { "topology" : definition.shell, "tight" : true });
        if (envelope.maxCorner[0] < 55 * millimeter)
            return;
        const sleeve = cylinder(context, id + "sleeve", AMBIENT_PORT - 8.6 * AMBIENT_DIRECTION,
            AMBIENT_PORT - 0.9 * AMBIENT_DIRECTION, 5.85);
        const cup = join(context, id + "integrateSleeve", [definition.shell, sleeve]);
        const pocket = cylinder(context, id + "capsulePocket", AMBIENT_PORT - 9.0 * AMBIENT_DIRECTION,
            AMBIENT_PORT - 3.35 * AMBIENT_DIRECTION, 5.12);
        const bore = cylinder(context, id + "acousticPort", AMBIENT_PORT - 3.45 * AMBIENT_DIRECTION,
            AMBIENT_PORT + 1.3 * AMBIENT_DIRECTION, 0.95);
        cut(context, id + "finishAmbientSeat", cup, [pocket, bore]);
    });
