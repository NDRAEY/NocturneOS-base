/// @param px - Point X
/// @param py - Point Y
/// @param rx - Rect X
/// @param ry - Rect Y
/// @param rw - Rect W
/// @param rh - Rect H
#[no_mangle]
pub extern "C" fn point_in_rect(
    px: isize,
    py: isize,
    rx: isize,
    ry: isize,
    rw: isize,
    rh: isize,
) -> bool {
    px >= rx && px <= rx + rw && py >= ry && py <= ry + rh
}
