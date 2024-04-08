__kernel void magnutude_filter_8u(
       __global const uchar* src, int src_step, int src_offset,
       __global uchar* dst, int dst_step, int dst_offset, int dst_rows, int dst_cols,
       float scale)
{
   int x = get_global_id(0);
   int y = get_global_id(1);
   if (x < dst_cols && y < dst_rows) {
       int dst_idx = y * dst_step + x + dst_offset;
       if (x > 0 && x < dst_cols - 1 && y > 0 && y < dst_rows - 2) {
           int src_idx = y * src_step + x + src_offset;
           int dx = (int)src[src_idx]*2 - src[src_idx - 1]          - src[src_idx + 1];
           int dy = (int)src[src_idx]*2 - src[src_idx - 1*src_step] - src[src_idx + 1*src_step];
           dst[dst_idx] = convert_uchar_sat(sqrt((float)(dx*dx + dy*dy)) * scale);
       } else {
           dst[dst_idx] = 0;
       }
   }
}
