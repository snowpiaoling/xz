
#include "simple_private.h"

static size_t
loongarch64_code(void *simple lzma_attribute((__unused__)),
                uint32_t now_pos, bool is_encoder,
                uint8_t *buffer, size_t size)
{
    size &= ~(size_t)3;

    size_t i;

    for(i=0; i<size; i += 4)
    {
        uint32_t pc = (uint32_t)(now_pos+i);
        uint32_t instr = read32le(buffer+i);

        if((instr >> 25) == 0x0A)
        {
            //LU12I.W
            uint32_t oprd = instr&0xFE00001F;
            uint32_t offset = (instr&0x01FFFFE0)<<7;

            pc &= 0xFFFFF000;
            if(!is_encoder)
            {
                pc = 0U-pc;
            }

            uint32_t addr = offset+pc;

            addr >>= 7;
            oprd |= (addr)&0x01FFFFE0;
            
            write32le(buffer+i, oprd);
        }else if((instr >> 26) == 0x14
                || (instr >> 26) == 0x15)
        {
            //B
            //BL
            uint32_t oprd = instr&0xFC000000;
            uint32_t offset = (instr&0x000003FF)<<16;
            offset |= (instr&0x03FFFC00)>>10;
            offset <<= 2;

            pc &= 0xFFFFFFFC;
            if(!is_encoder)
            {
                pc = 0U-pc;
            }

            uint32_t addr = offset+pc;

            addr >>= 2;
            oprd |= ((addr&0x0000FFFF)<<10)|((addr&0x03FF0000)>>16);

            write32le(buffer+i, oprd);
        }
    }

    return i;
}

static lzma_ret
loongarch64_coder_init(lzma_next_coder *next, const lzma_allocator *allocator,
        const lzma_filter_info *filters, bool is_encoder)
{
    return lzma_simple_coder_init(next, allocator, filters,
            &loongarch64_code, 0, 4, 4, is_encoder);
}

#ifdef HAVE_ENCODER_LOONGARCH64
extern lzma_ret
lzma_simple_loongarch64_encoder_init(lzma_next_coder *next,
		const lzma_allocator *allocator,
		const lzma_filter_info *filters)
{
	return loongarch64_coder_init(next, allocator, filters, true);
}


extern LZMA_API(size_t)
lzma_bcj_loongarch64_encode(uint32_t start_offset, uint8_t *buf, size_t size)
{
	// start_offset must be a multiple of four.
	start_offset &= ~UINT32_C(3);
	return loongarch64_code(NULL, start_offset, true, buf, size);
}
#endif

#ifdef HAVE_DECODER_LOONGARCH64
extern lzma_ret
lzma_simple_loongarch64_decoder_init(lzma_next_coder *next,
		const lzma_allocator *allocator,
		const lzma_filter_info *filters)
{
	return loongarch64_coder_init(next, allocator, filters, false);
}


extern LZMA_API(size_t)
lzma_bcj_loongarch64_decode(uint32_t start_offset, uint8_t *buf, size_t size)
{
	// start_offset must be a multiple of four.
	start_offset &= ~UINT32_C(3);
	return loongarch64_code(NULL, start_offset, false, buf, size);
}
#endif
