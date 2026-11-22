#!/usr/bin/env python3
"""Static format/memory checks, not a substitute for VRAM inspection in mGBA."""
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
actors = json.loads((root / 'assets/converted/player_sprites_manifest.json').read_text())
scene = json.loads((root / 'assets/converted/rooms/rm_w1_l1_scene_manifest.json').read_text())
room = json.loads((root / 'assets/converted/rooms/w1_l1_objects_report.json').read_text())
scene2 = json.loads((root / 'assets/converted/rooms/rm_w1_l2_scene_manifest.json').read_text())
room2 = json.loads((root / 'assets/converted/rooms/w1_l2_objects_report.json').read_text())
scene3 = json.loads((root / 'assets/converted/rooms/rm_w1_l3_scene_manifest.json').read_text())
room3 = json.loads((root / 'assets/converted/rooms/w1_l3_objects_report.json').read_text())
missing3 = json.loads((root / 'assets/converted/rooms/w1_l3_unimplemented.json').read_text())
boss_scene = json.loads((root / 'assets/converted/rooms/shrewy_scene_manifest.json').read_text())
boss_room = json.loads((root / 'assets/converted/rooms/shrewy_room_report.json').read_text())
boss_audit = json.loads((root / 'assets/converted/rooms/shrewy_source_audit.json').read_text())
boss_actors = json.loads((root / 'assets/converted/boss_sprites_manifest.json').read_text())
cage_sequence = json.loads((root / 'assets/converted/rooms/shrewy_cage_sequence_manifest.json').read_text())
cage_sequence_source = json.loads((root / 'assets/converted/rooms/shrewy_cage_sequence.json').read_text())
bonus_room = json.loads((root / 'assets/converted/rooms/bonus_w1_room_manifest.json').read_text())
bonus_trace = json.loads((root / 'assets/converted/rooms/bonus_w1_trace_summary.json').read_text())
bonus_collision = json.loads((root / 'assets/converted/rooms/bonus_w1_collision_report.json').read_text())
bonus_composite = json.loads((root / 'assets/converted/rooms/bonus_w1_composite_manifest.json').read_text())
native_scenes = {
    name: json.loads((root / 'assets/converted/rooms' / (name + '_native_manifest.json')).read_text())
    for name in ('w1_l1', 'w1_l2', 'w1_l3', 'bonus', 'shrewy')
}
oam_density = json.loads((root / 'assets/converted/rooms/bonus_w1_oam_density_native_v16.json').read_text())

assert actors['scale_divisor'] == 1
assert actors['opaque_palette_colors'] == 71
assert len(actors['sprites']) == 56
assert scene['unique_tiles'] <= 256
assert scene['tile_bytes'] <= 16384
assert scene['gba_scene'] == [1129, 180]
assert room['object_count'] == 214 and room['object_count'] <= 256
assert room['actors']['WORLD_SQUIRREL'] == 3
assert room['actors']['WORLD_SPRINGBOARD'] == 2
assert room['actors']['WORLD_ONION'] == 84
assert room['actors']['WORLD_HIVEFLY'] == 1
assert room['actors']['WORLD_PLANT'] == 63
assert scene2['unique_tiles'] <= 512
assert scene2['tile_bytes'] <= 32768
assert scene2['unique_tiles'] == 453
assert scene2['gba_scene'] == [1129,180]
assert room2['object_count'] == 367
assert room2['object_count'] < 384
assert room2['actors']['WORLD_ONION'] == 180
assert room2['actors']['WORLD_HIVEFLY'] == 2
assert room2['actors']['WORLD_BREAKABLE'] == 107
assert room2['actors']['WORLD_SPRINGBOARD'] == 4
assert room2['actors']['WORLD_SQUIRREL'] == 3
# BG char blocks 1+2 contain 32 KiB total; screenblocks 27/28 are outside.
assert 0x4000 + scene2['tile_bytes'] <= 0xC000
assert scene3['unique_tiles'] == 473
assert scene3['tile_bytes'] == 30272
assert scene3['unique_tiles'] <= 512 and 0x4000 + scene3['tile_bytes'] <= 0xC000
assert scene3['gba_scene'] == [1129,180]
assert room3['object_count'] == 502 and room3['object_count'] < 576
assert room3['actors']['WORLD_ONION'] == 318
assert room3['actors']['WORLD_BREAKABLE'] == 157
assert room3['actors']['WORLD_SPRINGBOARD'] == 3
assert room3['solid_instances'] == 78
assert room3['spawn_source_pivot'] == [49,382]
assert missing3['unimplemented_objects'] == 53
assert boss_scene['unique_tiles'] == 417
assert boss_scene['tile_bytes'] == 26688
assert boss_scene['gba_scene'] == [256, 144]
assert 0x4000 + boss_scene['tile_bytes'] <= 0xC000
assert boss_room['collision_grid'] == [32, 18]
assert boss_room['boss_source_pivot'] == [184, 104]
assert boss_room['player_source_pivot'] == [64, 160]
assert boss_room['floor_y_derived_from_owp_aabb'] == 112
assert boss_room['prop_count'] == 15
assert boss_room['props']['SHREWY_PROP_AUTODIGGER'] == 4
assert boss_room['props']['SHREWY_PROP_EMITTER'] == 4
assert boss_room['props']['SHREWY_PROP_AUTOHOLE'] == 2
assert boss_room['props']['SHREWY_PROP_ONION'] == 4
assert boss_room['props']['SHREWY_PROP_CAGE'] == 1
assert boss_audit['script_resource_count'] == 5510
assert boss_audit['code_chunk_present'] is False
assert boss_audit['sequence_resource_count'] == 7
assert 'seq_cage_drop' in boss_audit['sequence_resource_names']
assert boss_audit['room_instance_count'] == 24
assert boss_audit['room_object_counts']['obj_adjustable_autodigger'] == 4
assert boss_audit['trace_summary']['player_frames'] == 21304
assert cage_sequence_source['sequences'][0]['name'] == 'seq_cage_drop'
assert cage_sequence['playback_speed_fps'] == 60.0
assert cage_sequence['sequence_length_frames'] == 60
assert cage_sequence['position_key_length_frames'] == 67.0
assert len(cage_sequence['samples_q12']) == 60
assert cage_sequence['samples_source_pixels'][0] == -88.0
assert 30.0 < cage_sequence['samples_source_pixels'][20] < 31.0
assert 37.0 < cage_sequence['samples_source_pixels'][59] < 38.0
assert boss_actors['opaque_palette_colors'] <= 255

assert bonus_room['room'] == 'rm_w1_b1_00'
assert bonus_room['source_size'] == [5312, 1120]
assert bonus_room['gba_size'] == [2656, 560]
assert bonus_room['map_size'] == [332, 70]
assert bonus_room['visible_tile_layers'] == ['Tiles_1', 'TilesFront']
assert bonus_room['unique_gba_tiles'] == 139
assert bonus_room['tile_bytes'] == 8896 and bonus_room['tile_bytes'] <= 16384
assert bonus_room['palette_opaque_colors'] == 22
assert bonus_room['instance_count'] == 1024
assert bonus_room['object_kind_count'] == 46
assert bonus_room['object_counts']['obj_onion'] == 397
assert bonus_room['object_counts']['obj_destructible_block'] == 193
assert bonus_room['object_counts']['obj_race_start'] == 2
assert bonus_room['object_counts']['obj_race_finish'] == 1
assert bonus_room['object_counts']['obj_race_podium'] == 1
assert bonus_room['object_counts']['obj_bunny'] == 2
assert len(bonus_room['background_layers']) == 4
assert bonus_trace['total_player_frames'] == 2983
assert bonus_trace['total_event_records'] == 478
assert bonus_trace['event_types']['race'] == 1
assert bonus_trace['race_timing_trace_estimate']['gate_to_finish_elapsed_ms'] is None
assert bonus_trace['race_timing_trace_estimate']['final_clip_start_to_finish_event_ms'] == 33682
assert bonus_trace['swole_frames'] == 0
assert len(bonus_trace['segments']) == 4
assert bonus_collision['collision_grid'] == [332, 70]
assert bonus_composite['map'] == [332, 70]
assert bonus_composite['unique_tiles'] == 486
assert bonus_composite['tile_bytes'] == 31104
assert bonus_composite['opaque_colors'] == 24
assert bonus_composite['fits_two_8bpp_bg_charblocks']
assert bonus_composite['tile_bytes'] <= 32768
assert bonus_collision['object_placements_used']['obj_solid'] == 166
assert bonus_collision['object_placements_used']['obj_destructible_block'] == 193
assert bonus_collision['object_placements_used']['obj_cloud_platform'] == 7
assert 'obj_moving_platform' not in bonus_collision['object_placements_used']
assert bonus_collision['object_placements_used']['obj_destructible_block_oneway'] == 4
assert bonus_collision['collision_cell_counts']['7'] == 36
assert bonus_collision['collision_cell_counts']['8'] == 4
assert bonus_collision['collision_cell_counts']['9'] == 8
assert oam_density['source_objects_tracked'] == 793
assert oam_density['source_object_counts']['obj_bunny'] == 2
assert oam_density['source_object_counts']['obj_bunny_ground'] == 9
assert oam_density['max_hardware_oam_estimate_including_8_player_parts'] <= 128
assert oam_density['max_hardware_oam_estimate_including_8_player_parts'] == 66
assert oam_density['camera_grid_positions_evaluated'] > 4800000

world_oam = {
    name: json.loads((root / 'assets/converted/rooms' /
                      (name + '_oam_density_native_v16.json')).read_text())
    for name in ('w1_l1', 'w1_l2', 'w1_l3')
}
assert world_oam['w1_l1']['max_hardware_oam_estimate_including_player'] == 58
assert world_oam['w1_l2']['max_hardware_oam_estimate_including_player'] == 66
assert world_oam['w1_l3']['max_hardware_oam_estimate_including_player'] == 82
assert all(item['fits_128_obj_entries'] for item in world_oam.values())
assert bonus_collision['collision_cell_counts']['2'] == 271
assert 0x4000 + bonus_room['tile_bytes'] <= 0xC000


by_name = {a['name']: a for a in actors['sprites']}
required = (
    'spr_onion', 'spr_destructible_block', 'spr_action_block',
    'spr_action_block_spent', 'spr_squirrel_idle', 'spr_squirrel_walk',
    'spr_springboard', 'spr_destructible_block_swolvy', 'spr_bunny_hop',
    'spr_bumper_sq', 'spr_bumper_tri', 'spr_moving_platform',
    'spr_moving_platform_wide', 'spr_cloud_platform', 'spr_hivefly',
    'spr_crowly_fly', 'spr_vinechomp_base', 'spr_spikes_dave',
    'spr_cannon', 'spr_button_off', 'spr_rail_slice',
    'spr_bkg_space_onion', 'spr_worm', 'spr_plants', 'spr_level_exit_hole'
)
static_bytes = 0
base32 = 160  # 5120B reserved for native player frames
for name in required:
    assert name in by_name, f'missing generated actor animation {name}'
    animation = by_name[name]
    max_parts = max(len(frame['parts']) for frame in animation['frames'])
    assert max_parts <= 8
    for part_index in range(max_parts):
        capacity = max(frame['parts'][part_index]['tile_bytes']
                       if part_index < len(frame['parts']) else 0
                       for frame in animation['frames'])
        assert capacity % 64 == 0
        static_bytes += capacity
        if base32 & 1:
            base32 += 1
        base32 += capacity // 32

# Actor animation cache stores one frame per animation and streams frame
# changes into fixed slots, instead of reserving VRAM for every animation frame.
max_player_frame_bytes = max(
    sum(part['tile_bytes'] for part in frame['parts'])
    for actor in actors['sprites']
    if actor['name'] not in required
    for frame in actor['frames']
)
assert max_player_frame_bytes <= 5120
assert static_bytes == 18560
assert base32 == 740
assert base32 * 32 <= 32768


# Native-resolution Shrewy arena uses a separate OBJ palette and mode-specific
# OBJ VRAM windows: player 0..4KiB, onion props 4..5KiB, boss 8..11KiB,
# and one rock frame at 24..25KiB. The world actor cache is restored on exit.
boss_by_name = {a['name']: a for a in boss_actors['sprites']}
for name in ('spr_idle', 'spr_volvy_hurt', 'spr_swolvy_idle', 'spr_dig',
             'spr_dash', 'spr_grind', 'spr_swolvy_grind', 'spr_dig_swole', 'spr_onion',
             'spr_shrewy_idle', 'spr_shrewy_windup', 'spr_shrewy_jump',
             'spr_shrewy_stomp', 'spr_shrewy_dropkick', 'spr_shrewy_tornado',
             'spr_shrewy_hurt', 'spr_shrewy_rock'):
    assert name in boss_by_name, f'missing native boss-mode sprite {name}'

def max_frame_bytes(sprite):
    return max(sum(part['tile_bytes'] for part in frame['parts'])
               for frame in sprite['frames'])

boss_player_names = ('spr_idle', 'spr_walk', 'spr_jump', 'spr_float',
                     'spr_ground_pound', 'spr_punch', 'spr_punch_up', 'spr_dash',
                     'spr_dash_up', 'spr_grind', 'spr_dig', 'spr_dig_swole',
                     'spr_volvy_hurt', 'spr_swolvy_idle', 'spr_swolvy_walk',
                     'spr_swolvy_jump', 'spr_swolvy_float', 'spr_swolvy_grind',
                     'spr_swolvy_ground_pound', 'spr_swolvy_punch',
                     'spr_swolvy_punch_up')
boss_player_max = max(max_frame_bytes(boss_by_name[n]) for n in boss_player_names)
boss_body_names = ('spr_shrewy_idle', 'spr_shrewy_windup', 'spr_shrewy_jump',
                   'spr_shrewy_stomp', 'spr_shrewy_dropkick',
                   'spr_shrewy_tornado', 'spr_shrewy_hurt')
boss_body_max = max(max_frame_bytes(boss_by_name[n]) for n in boss_body_names)
rock_max = max_frame_bytes(boss_by_name['spr_shrewy_rock'])
onion_total = sum(sum(part['tile_bytes'] for part in frame['parts'])
                   for frame in boss_by_name['spr_onion']['frames'])
assert boss_player_max <= 4096
assert boss_body_max <= 4096
assert rock_max <= 1024
assert onion_total <= 1024
assert 0 + boss_player_max <= 4096
assert 4096 + onion_total <= 8192
assert 8192 + boss_body_max <= 16384
assert 24576 + rock_max <= 32768

print('Asset layout tests: PASS '
      f'(player max {max_player_frame_bytes}B, static actors {static_bytes}B, '
      f'BG L1 {scene["tile_bytes"]}B; BG L2 {scene2["tile_bytes"]}B; BG L3 {scene3["tile_bytes"]}B; Shrewy BG {boss_scene["tile_bytes"]}B; Bonus tile-only {bonus_room["tile_bytes"]}B; Bonus composite {bonus_composite["tile_bytes"]}B; boss OBJ max {boss_body_max}B)')
