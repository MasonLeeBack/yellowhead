#pragma once

typedef s64 CalendarTime;

bool TryGetCalendarTime(CalendarTime *timestamp);
CalendarTime GetCalendarTime();
CalendarTime GetCachedCalendarTime();
void SetServerTimestamp(CalendarTime server_timestamp);
s64 GetServerTimestampDelta();
