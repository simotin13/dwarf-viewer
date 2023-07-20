class BinUtil
{
public:
    static uint16_t FromLeToUInt16(const uint8_t *buf)
    {
        return (((uint16_t)buf[1] << 8) + (uint16_t)buf[0]);
    }

    static int16_t FromLeToInt16(const uint8_t *buf)
    {
        return (((int16_t)buf[1] << 8) + (int16_t)buf[0]);
    }

    static uint32_t FromLeToUInt32(const uint8_t *buf)
    {
        return (((uint32_t)buf[3] << 24) + ((uint32_t)buf[2] << 16) + ((uint32_t)buf[1] << 8) + (uint32_t)buf[0]);
    }
    static int32_t FromLeToInt32(const uint8_t *buf)
    {
        return (((int32_t)buf[3] << 24) + ((int32_t)buf[2] << 16) + ((int32_t)buf[1] << 8) + (int32_t)buf[0]);
    }

    static uint64_t FromLeToUInt64(const uint8_t *buf)
    {
        uint64_t val = 0;
        val += ((uint64_t)buf[7] << 56);
        val += ((uint64_t)buf[6] << 48);
        val += ((uint64_t)buf[5] << 40);
        val += ((uint64_t)buf[4] << 32);
        val += ((uint64_t)buf[3] << 24);
        val += ((uint64_t)buf[2] << 16);
        val += ((uint64_t)buf[1] << 8);
        val += buf[0];
        return val;
    }

    static int64_t FromLeToInt64(const uint8_t *buf)
    {
        int64_t val = 0;
        val += ((int64_t)buf[7] << 56);
        val += ((int64_t)buf[6] << 48);
        val += ((int64_t)buf[5] << 40);
        val += ((int64_t)buf[4] << 32);
        val += ((int64_t)buf[3] << 24);
        val += ((int64_t)buf[2] << 16);
        val += ((int64_t)buf[1] << 8);
        val += buf[0];
        return val;
    }

    static std::string GetString(const uint8_t *buf, uint64_t size, uint64_t pos)
    {
        std::string s;
        while(pos < size)
        {
            if (buf[pos] == 0)
            {
                break;
            }
            s += (char)buf[pos];
            pos++;
        }
        return s;
    }
};
