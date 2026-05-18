#!/usr/bin/env python3
"""Convert Shruthi-1 patch banks to Ambika softsynth C++ patch arrays."""

# ---------------------------------------------------------------------------
# Shruthi-1 enum mappings (from utils/convert_shruthi_data.py)
# ---------------------------------------------------------------------------

SHRUTHI_MOD_SOURCE = [
  'MOD_SRC_LFO_1', 'MOD_SRC_LFO_2', 'MOD_SRC_SEQ', 'MOD_SRC_SEQ_1',
  'MOD_SRC_SEQ_2', 'MOD_SRC_STEP', 'MOD_SRC_WHEEL', 'MOD_SRC_AFTERTOUCH',
  'MOD_SRC_PITCH_BEND', 'MOD_SRC_OFFSET', 'MOD_SRC_CV_1', 'MOD_SRC_CV_2',
  'MOD_SRC_CV_3', 'MOD_SRC_CV_4', 'MOD_SRC_CC_A', 'MOD_SRC_CC_B',
  'MOD_SRC_CC_C', 'MOD_SRC_CC_D', 'MOD_SRC_NOISE', 'MOD_SRC_ENV_1',
  'MOD_SRC_ENV_2', 'MOD_SRC_VELOCITY', 'MOD_SRC_RANDOM', 'MOD_SRC_NOTE',
  'MOD_SRC_GATE', 'MOD_SRC_AUDIO', 'MOD_SRC_OP_1', 'MOD_SRC_OP_2',
  'MOD_SRC_TRIG_1', 'MOD_SRC_TRIG_2', 'MOD_SRC_CONSTANT_4',
  'MOD_SRC_CONSTANT_8', 'MOD_SRC_CONSTANT_16', 'MOD_SRC_CONSTANT_32',
]

SHRUTHI_MOD_DESTINATION = [
  'MOD_DST_FILTER_CUTOFF', 'MOD_DST_VCA', 'MOD_DST_PARAMETER_1',
  'MOD_DST_PARAMETER_2', 'MOD_DST_OSC_1', 'MOD_DST_OSC_2',
  'MOD_DST_OSC_1_2_COARSE', 'MOD_DST_OSC_1_2_FINE', 'MOD_DST_MIX_BALANCE',
  'MOD_DST_MIX_NOISE', 'MOD_DST_MIX_SUB_OSC', 'MOD_DST_FILTER_RESONANCE',
  'MOD_DST_CV_1', 'MOD_DST_CV_2', 'MOD_DST_ATTACK', 'MOD_DST_LFO_1',
  'MOD_DST_LFO_2', 'MOD_DST_TRIGGER_ENV_1', 'MOD_DST_TRIGGER_ENV_2',
]

SHRUTHI_OSCILLATOR_ALGORITHM = [
  'WAVEFORM_NONE', 'WAVEFORM_SAW', 'WAVEFORM_SQUARE', 'WAVEFORM_TRIANGLE',
  'WAVEFORM_CZ_SAW', 'WAVEFORM_CZ_RESO',
  'WAVEFORM_CZ_TRIANGLE', 'WAVEFORM_CZ_PULSE',
  'WAVEFORM_CZ_SYNC', 'WAVEFORM_QUAD_SAW_PAD',
  'WAVEFORM_FM', 'WAVEFORM_WAVETABLE_WAVES',
  'WAVEFORM_WAVETABLE_TAMPURA', 'WAVEFORM_WAVETABLE_DIGITAL',
  'WAVEFORM_WAVETABLE_METALLIC', 'WAVEFORM_WAVETABLE_BOWED',
  'WAVEFORM_WAVETABLE_SLAP', 'WAVEFORM_WAVETABLE_ORGAN',
  'WAVEFORM_WAVETABLE_MALE', 'WAVEFORM_WAVETABLE_USER',
  'WAVEFORM_8BITLAND', 'WAVEFORM_CRUSHED_SINE',
  'WAVEFORM_DIRTY_PWM', 'WAVEFORM_FILTERED_NOISE',
  'WAVEFORM_VOWEL', 'WAVEFORM_WAVETABLE_BELLISH',
  'WAVEFORM_WAVETABLE_POLATED', 'WAVEFORM_WAVETABLE_CELLO',
  'WAVEFORM_WAVETABLE_CLIPSWEEP', 'WAVEFORM_WAVETABLE_FEMALE',
  'WAVEFORM_WAVETABLE_FMNTVOC', 'WAVEFORM_WAVETABLE_FORMANT2',
  'WAVEFORM_WAVETABLE_RES3HP', 'WAVEFORM_WAVETABLE_ELECTP',
  'WAVEFORM_WAVETABLE_VIBES',
]

SHRUTHI_MIX_OPERATOR = [
  'OP_SUM', 'OP_SYNC', 'OP_RING_MOD', 'OP_XOR', 'OP_FUZZ',
  'OP_CRUSH_4', 'OP_CRUSH_8', 'OP_FOLD', 'OP_BITS', 'OP_DUO',
  'OP_PING_PONG_2', 'OP_PING_PONG_4', 'OP_PING_PONG_8', 'OP_PING_PONG_SEQ',
]

AMBIKA_OSCILLATOR_ALGORITHM = {
  'WAVEFORM_NONE': 0, 'WAVEFORM_SAW': 1, 'WAVEFORM_SQUARE': 2,
  'WAVEFORM_TRIANGLE': 3, 'WAVEFORM_CRUSHED_SINE': 4,
  'WAVEFORM_CZ_SAW': 5, 'WAVEFORM_CZ_SAW_LP': 6, 'WAVEFORM_CZ_SAW_PK': 7,
  'WAVEFORM_CZ_SAW_BP': 8, 'WAVEFORM_CZ_SAW_HP': 9, 'WAVEFORM_CZ_PLS_LP': 10,
  'WAVEFORM_CZ_PLS_PK': 11, 'WAVEFORM_CZ_PLS_BP': 12,
  'WAVEFORM_CZ_PLS_HP': 13, 'WAVEFORM_CZ_TRI_LP': 14,
  'WAVEFORM_QUAD_SAW_PAD': 15, 'WAVEFORM_FM': 16, 'WAVEFORM_8BITLAND': 17,
  'WAVEFORM_DIRTY_PWM': 18, 'WAVEFORM_FILTERED_NOISE': 19,
  'WAVEFORM_VOWEL': 20, 'WAVEFORM_WAVETABLE_MALE': 21,
  'WAVEFORM_WAVETABLE_FEMALE': 22, 'WAVEFORM_WAVETABLE_FMNTVOC': 23,
  'WAVEFORM_WAVETABLE_TAMPURA': 24, 'WAVEFORM_WAVETABLE_BOWED': 25,
  'WAVEFORM_WAVETABLE_CELLO': 26, 'WAVEFORM_WAVETABLE_VIBES': 27,
  'WAVEFORM_WAVETABLE_SLAP': 28, 'WAVEFORM_WAVETABLE_ELECTP': 29,
  'WAVEFORM_WAVETABLE_ORGAN': 30, 'WAVEFORM_WAVETABLE_WAVES': 31,
  'WAVEFORM_WAVETABLE_DIGITAL': 32, 'WAVEFORM_WAVETABLE_POLATED': 33,
  'WAVEFORM_WAVETABLE_FORMANT2': 34, 'WAVEFORM_WAVETABLE_CLIPSWEEP': 34,
  'WAVEFORM_WAVETABLE_METALLIC': 35, 'WAVEFORM_WAVETABLE_BELLISH': 36,
  'WAVEFORM_WAVETABLE_WAVEQUENCE': 37, 'WAVEFORM_WAVETABLE_USER': 31,
  'WAVEFORM_WAVETABLE_RES3HP': 9, 'WAVEFORM_CZ_RESO': 6,
  'WAVEFORM_CZ_TRIANGLE': 14, 'WAVEFORM_CZ_PULSE': 11,
  'WAVEFORM_CZ_SYNC': 12,
}

AMBIKA_MOD_SRC = {
  'MOD_SRC_ENV_1': 1, 'MOD_SRC_ENV_2': 2, 'MOD_SRC_ENV_3': 0,
  'MOD_SRC_LFO_1': 3, 'MOD_SRC_LFO_2': 4, 'MOD_SRC_LFO_3': 5,
  'MOD_SRC_LFO_4': 6, 'MOD_SRC_OP_1': 7, 'MOD_SRC_OP_2': 8,
  'MOD_SRC_OP_3': 9, 'MOD_SRC_OP_4': 10, 'MOD_SRC_SEQ_1': 11,
  'MOD_SRC_SEQ_2': 12, 'MOD_SRC_ARP_STEP': 13, 'MOD_SRC_VELOCITY': 14,
  'MOD_SRC_AFTERTOUCH': 15, 'MOD_SRC_PITCH_BEND': 16, 'MOD_SRC_WHEEL': 17,
  'MOD_SRC_WHEEL_2': 18, 'MOD_SRC_EXPRESSION': 19, 'MOD_SRC_NOTE': 20,
  'MOD_SRC_GATE': 21, 'MOD_SRC_NOISE': 22, 'MOD_SRC_RANDOM': 23,
  'MOD_SRC_CONSTANT_256': 24, 'MOD_SRC_CONSTANT_128': 25,
  'MOD_SRC_CONSTANT_64': 26, 'MOD_SRC_CONSTANT_32': 27,
  'MOD_SRC_CONSTANT_16': 28, 'MOD_SRC_CONSTANT_8': 29,
  'MOD_SRC_CONSTANT_4': 30,
  'MOD_SRC_SEQ': 11, 'MOD_SRC_STEP': 13, 'MOD_SRC_AUDIO': 29,
  'MOD_SRC_OFFSET': 24,
}

AMBIKA_MOD_DST = {
  'MOD_DST_PARAMETER_1': 0, 'MOD_DST_PARAMETER_2': 1,
  'MOD_DST_OSC_1': 2, 'MOD_DST_OSC_2': 3,
  'MOD_DST_OSC_1_2_COARSE': 4, 'MOD_DST_OSC_1_2_FINE': 5,
  'MOD_DST_MIX_BALANCE': 6, 'MOD_DST_MIX_PARAM': 7,
  'MOD_DST_MIX_NOISE': 8, 'MOD_DST_MIX_SUB_OSC': 9,
  'MOD_DST_MIX_FUZZ': 10, 'MOD_DST_MIX_CRUSH': 11,
  'MOD_DST_FILTER_CUTOFF': 12, 'MOD_DST_FILTER_RESONANCE': 13,
  'MOD_DST_ATTACK': 14, 'MOD_DST_DECAY': 15, 'MOD_DST_RELEASE': 16,
  'MOD_DST_LFO_4': 17, 'MOD_DST_VCA': 18,
  'MOD_DST_LFO_1': 17, 'MOD_DST_LFO_2': 17,
}

AMBIKA_SUB_OSC_SHAPES = {
  0: 'WAVEFORM_SUB_OSC_SQUARE_1', 1: 'WAVEFORM_SUB_OSC_TRIANGLE_1',
  2: 'WAVEFORM_SUB_OSC_PULSE_1', 3: 'WAVEFORM_SUB_OSC_SQUARE_2',
  4: 'WAVEFORM_SUB_OSC_TRIANGLE_2', 5: 'WAVEFORM_SUB_OSC_PULSE_2',
  6: 'WAVEFORM_SUB_OSC_CLICK', 7: 'WAVEFORM_SUB_OSC_GLITCH',
  8: 'WAVEFORM_SUB_OSC_BLOW', 9: 'WAVEFORM_SUB_OSC_METALLIC',
  10: 'WAVEFORM_SUB_OSC_POP',
}


def parse_hex(hex_str):
    return [int(hex_str[i:i+2], 16) for i in range(0, len(hex_str), 2)]


def map_osc_shape(shruthi_shape):
    name = SHRUTHI_OSCILLATOR_ALGORITHM[shruthi_shape]
    return AMBIKA_OSCILLATOR_ALGORITHM.get(name, 0)


def map_mod_source(shruthi_src):
    name = SHRUTHI_MOD_SOURCE[shruthi_src]
    return AMBIKA_MOD_SRC.get(name, 3)


def map_mod_dest(shruthi_dst):
    name = SHRUTHI_MOD_DESTINATION[shruthi_dst]
    return AMBIKA_MOD_DST.get(name, 12)


def convert_patch(bytes, name):
    patch = [0] * 112

    # Oscillators (0-7)
    patch[0] = map_osc_shape(bytes[0])
    patch[1] = bytes[1]
    patch[2] = bytes[2]
    patch[3] = 0
    patch[4] = map_osc_shape(bytes[4])
    patch[5] = bytes[5]
    patch[6] = bytes[6]
    patch[7] = bytes[7]

    # Mix (8-15)
    op = SHRUTHI_MIX_OPERATOR[bytes[3]]
    bal = bytes[8]
    if op == 'OP_SUM':
        patch[8] = bal; patch[9] = 0; patch[10] = 0; patch[14] = 0; patch[15] = 0
    elif op == 'OP_SYNC':
        patch[8] = bal; patch[9] = 1; patch[10] = 0; patch[14] = 0; patch[15] = 0
    elif op == 'OP_RING_MOD':
        patch[8] = 0; patch[9] = 2; patch[10] = bal; patch[14] = 0; patch[15] = 0
    elif op == 'OP_XOR':
        patch[8] = 31; patch[9] = 3; patch[10] = 63; patch[14] = 0; patch[15] = 0
    elif op == 'OP_FUZZ':
        patch[8] = 31; patch[9] = 0; patch[10] = 0; patch[14] = bal; patch[15] = 0
    elif op == 'OP_CRUSH_4':
        patch[8] = bal; patch[9] = 0; patch[10] = 0; patch[14] = 0; patch[15] = 4
    elif op == 'OP_CRUSH_8':
        patch[8] = bal; patch[9] = 0; patch[10] = 0; patch[14] = 0; patch[15] = 8
    elif op == 'OP_FOLD':
        patch[8] = 31; patch[9] = 4; patch[10] = bal; patch[14] = 0; patch[15] = 0
    elif op == 'OP_BITS':
        patch[8] = 31; patch[9] = 5; patch[10] = bal; patch[14] = 0; patch[15] = 0
    else:
        patch[8] = bal; patch[9] = 0; patch[10] = 0; patch[14] = 0; patch[15] = 0

    patch[11] = bytes[11]
    patch[12] = bytes[9]
    patch[13] = bytes[10]

    # Filter (16-23)
    patch[16] = bytes[12]
    patch[17] = bytes[13]
    patch[18] = 0
    patch[19] = 0
    patch[20] = 0
    patch[21] = 0
    patch[22] = bytes[14]
    patch[23] = bytes[15]

    # Env/LFO slots (24-47)
    # Slot 0: env[0] parameters + lfo[0] waveform/rate
    patch[24] = bytes[16]
    patch[25] = bytes[17]
    patch[26] = bytes[18]
    patch[27] = bytes[19]
    patch[28] = bytes[24]
    lfo0_rate = bytes[25]
    if lfo0_rate <= 15:
        lfo0_rate = 48
    patch[29] = lfo0_rate
    patch[30] = 0
    patch[31] = bytes[27]

    # Slot 1: env[1] + lfo[1]
    patch[32] = bytes[20]
    patch[33] = bytes[21]
    patch[34] = bytes[22]
    patch[35] = bytes[23]
    patch[36] = bytes[28]
    lfo1_rate = bytes[29]
    if lfo1_rate <= 15:
        lfo1_rate = 48
    patch[37] = lfo1_rate
    patch[38] = 0
    patch[39] = bytes[31]

    # Slot 2: default env + lfo
    patch[40] = 0
    patch[41] = 40
    patch[42] = 80
    patch[43] = 40
    patch[44] = 0
    patch[45] = 63
    patch[46] = 0
    patch[47] = 0

    # Voice LFO (48-49)
    patch[48] = 0
    patch[49] = lfo0_rate

    # Modulations (50-91)
    for i in range(14):
        if i < 6:
            idx = i
        elif i < 8:
            src = 3
            dst = 12
            amount = 0
            patch[50 + i * 3] = src
            patch[51 + i * 3] = dst
            patch[52 + i * 3] = amount
            continue
        else:
            idx = i - 2
        src_name = SHRUTHI_MOD_SOURCE[bytes[32 + idx * 3]]
        dst_name = SHRUTHI_MOD_DESTINATION[bytes[33 + idx * 3]]
        amount = bytes[34 + idx * 3]
        if dst_name == 'MOD_DST_ATTACK' and amount != 0:
            amount = 256 - amount
        if src_name not in AMBIKA_MOD_SRC or dst_name not in AMBIKA_MOD_DST:
            src = 3
            dst = 12
            amount = 0
        else:
            src = AMBIKA_MOD_SRC[src_name]
            dst = AMBIKA_MOD_DST[dst_name]
        patch[50 + i * 3] = src
        patch[51 + i * 3] = dst
        patch[52 + i * 3] = amount

    # Default modifiers (92-103)
    patch[92] = 3; patch[93] = 4; patch[94] = 0
    patch[95] = 4; patch[96] = 5; patch[97] = 0
    patch[98] = 5; patch[99] = 6; patch[100] = 0
    patch[101] = 6; patch[102] = 7; patch[103] = 0

    # Padding (104-111)
    for i in range(104, 112):
        patch[i] = 0

    return patch


def patch_to_c(name, data):
    """Format a Patch as a C++ aggregate initializer."""
    lines = []
    # Oscillators
    lines.append(f'  // {name}')
    lines.append('  // Oscillators')
    lines.append(f'  /* osc[0] */  {data[0]:3d}, {data[1]:3d}, {data[2]:3d}, {data[3]:3d},')
    lines.append(f'  /* osc[1] */  {data[4]:3d}, {data[5]:3d}, {data[6]:3d}, {data[7]:3d},')
    # Mix
    lines.append(f'  // Mix')
    lines.append(f'  /* mix   */  {data[8]:3d}, {data[9]:3d}, {data[10]:3d}, {data[11]:3d},'
                 f' {data[12]:3d}, {data[13]:3d}, {data[14]:3d}, {data[15]:3d},')
    # Filter
    lines.append(f'  // Filter')
    lines.append(f'  /* filt  */  {data[16]:3d}, {data[17]:3d}, {data[18]:3d},'
                 f' {data[19]:3d}, {data[20]:3d}, {data[21]:3d},'
                 f' {data[22]:3d}, {data[23]:3d},')
    # Env/LFO slots
    lines.append(f'  // Envelope/LFO slots')
    for s in range(3):
        off = 24 + s * 8
        lines.append(f'  /* env{ s} */  {data[off]:3d}, {data[off+1]:3d}, {data[off+2]:3d},'
                     f' {data[off+3]:3d}, {data[off+4]:3d}, {data[off+5]:3d},'
                     f' {data[off+6]:3d}, {data[off+7]:3d},')
    # Voice LFO
    lines.append(f'  // Voice LFO')
    lines.append(f'  /* vlfo  */  {data[48]:3d}, {data[49]:3d},')
    # Modulations
    lines.append(f'  // Modulations')
    for i in range(14):
        off = 50 + i * 3
        lines.append(f'  /* mod{ i:2d} */  {data[off]:3d}, {data[off+1]:3d}, {data[off+2]:3d},')
    # Modifiers
    lines.append(f'  // Modifiers')
    for i in range(4):
        off = 92 + i * 3
        lines.append(f'  /* modf{ i} */  {data[off]:3d}, {data[off+1]:3d}, {data[off+2]:3d},')
    # Padding
    lines.append(f'  // Padding')
    lines.append(f'  /* pad   */  ' + ', '.join(f'{data[i]:3d}' for i in range(104, 112)) + ',')
    return '\n'.join(lines)


def generate_header(patches, names):
    lines = []
    lines.append('// Auto-generated by tools/convert_patches.py')
    lines.append('// Ambika softsynth patch bank — converted from Shruthi-1 format')
    lines.append('#ifndef AMBIKA_PATCHES_H_')
    lines.append('#define AMBIKA_PATCHES_H_')
    lines.append('')
    lines.append('#include "common/patch.h"')
    lines.append('')
    lines.append('namespace ambika {')
    lines.append('')
    lines.append(f'extern const Patch kPatches[{len(patches)}];')
    lines.append(f'extern const char* const kPatchNames[{len(patches)}];')
    lines.append(f'extern const int kNumPatches;')
    lines.append('')
    lines.append('}  // namespace ambika')
    lines.append('')
    lines.append('#endif  // AMBIKA_PATCHES_H_')
    return '\n'.join(lines)


def generate_source(patches, names):
    lines = []
    lines.append('// Auto-generated by tools/convert_patches.py')
    lines.append('// Ambika softsynth patch bank — converted from Shruthi-1 format')
    lines.append('#include "patches.h"')
    lines.append('')
    lines.append('namespace ambika {')
    lines.append('')
    lines.append('const Patch kPatches[] = {')
    for i, (p, n) in enumerate(zip(patches, names)):
        lines.append('{')
        lines.append(patch_to_c(n, p))
        lines.append('},')
    lines.append('};')
    lines.append('')
    lines.append(f'const int kNumPatches = {len(patches)};')
    lines.append('')
    lines.append('const char* const kPatchNames[] = {')
    for n in names:
        lines.append(f'  "{n}",')
    lines.append('};')
    lines.append('')
    lines.append('}  // namespace ambika')
    return '\n'.join(lines)


def parse_patch_file(path):
    patches = []
    names = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            tokens = line.split('\t')
            if tokens[0] != 'patch':
                continue
            name = tokens[1].strip()
            hex_str = ''.join(tokens[2:])
            raw = parse_hex(hex_str)
            patch = convert_patch(raw, name)
            patches.append(patch)
            names.append(name)
    return patches, names


def main():
    import sys
    src_dir = sys.argv[1] if len(sys.argv) > 1 else '../utils'
    out_dir = sys.argv[2] if len(sys.argv) > 2 else '..'

    # Parse both patch banks
    p1, n1 = parse_patch_file(f'{src_dir}/shruthi_patches.txt')
    p2, n2 = parse_patch_file(f'{src_dir}/shruthi_patches_funkybank.txt')
    patches = p1 + p2
    names = n1 + n2

    print(f'Converted {len(patches)} patches ({len(p1)} + {len(p2)})')

    header = generate_header(patches, names)
    with open(f'{out_dir}/patches.h', 'w') as f:
        f.write(header)

    source = generate_source(patches, names)
    with open(f'{out_dir}/patches.cc', 'w') as f:
        f.write(source)

    print(f'Wrote {out_dir}/patches.h and {out_dir}/patches.cc')


if __name__ == '__main__':
    main()
