#include "types.h"
#include "CalendarTime.h"

#include <cell/rtc.h>
#include <time.h>

s64 GServerTimestampDelta;

extern u32 GGraphicsFrameNum;

CalendarTime GetCalendarTime()
{
    CalendarTime r = 0;
    TryGetCalendarTime(&r);
    return r;
}

bool TryGetCalendarTime(CalendarTime *timestamp)
{
    int r;
    CellRtcDateTime rtc_time_utc;

    r = cellRtcGetCurrentClock(&rtc_time_utc, 0);
    if (r != 0)
        return false;

    time_t time_out;
    r = cellRtcGetTime_t(&rtc_time_utc, &time_out);
    if (r != 0)
        return false;

    *timestamp = time_out;
    return true;
}

CalendarTime GetCachedCalendarTime()
{
    static s64 last_calendartime;
    static u32 last_calendartime_frame;

    if (last_calendartime_frame != GGraphicsFrameNum) {
        last_calendartime_frame = GGraphicsFrameNum;
        last_calendartime = GetCalendarTime();
    }

    return last_calendartime;
}

void SetServerTimestamp(CalendarTime server_timestamp)
{
    if (server_timestamp != 0) {
        CalendarTime local_timestamp = 0;
        if (TryGetCalendarTime(&local_timestamp)) {
            s64 delta = local_timestamp - server_timestamp;
            GServerTimestampDelta = delta;
            return;
        }
    }

    GServerTimestampDelta = 0;
}

s64 GetServerTimestampDelta()
{
    return GServerTimestampDelta;
}
