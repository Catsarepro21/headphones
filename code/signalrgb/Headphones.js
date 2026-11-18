export function VendorId() { return 0x0000; }
export function ProductId() { return 0x0000; }
export function Name() { return "Headphones"; }
export function Type() { return "hid"; }
export function Size() { return [23, 13]; }

const cupCount = 2;
const ledsPerCup = 16;
const positions = [];
const names = [];
let sequence = 0;

for (let cup = 0; cup < cupCount; cup++) {
    for (let pixel = 0; pixel < ledsPerCup; pixel++) {
        const angle = 2 * Math.PI * pixel / ledsPerCup;
        positions.push([
            Math.round(5 + 12 * cup + 4 * Math.sin(angle)),
            Math.round(6 - 6 * Math.cos(angle)),
        ]);
        names.push(`${cup === 0 ? "Left" : "Right"} ${pixel + 1}`);
    }
}

export function LedNames() { return names; }
export function LedPositions() { return positions; }

export function Validate(endpoint) {
    return VendorId() !== 0 && ProductId() !== 0 && endpoint.usage_page === 0xff31;
}

function sendFrame(black) {
    sequence = (sequence + 1) & 0xff;
    for (let cup = 0; cup < cupCount; cup++) {
        const report = [1, 0x52, sequence, cup];
        for (let pixel = 0; pixel < ledsPerCup; pixel++) {
            const [x, y] = positions[cup * ledsPerCup + pixel];
            const rgb = black ? [0, 0, 0] : device.color(x, y);
            report.push(rgb[0], rgb[1], rgb[2]);
        }
        device.send_report(report, report.length);
    }
}

export function Initialize() { sendFrame(true); }
export function Render() { sendFrame(false); }
export function Shutdown() { sendFrame(true); }
