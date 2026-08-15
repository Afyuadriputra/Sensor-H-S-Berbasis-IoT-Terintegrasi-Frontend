import React, { useState, useMemo } from "react";
import {
  ResponsiveContainer,
  AreaChart,
  Area,
  XAxis,
  YAxis,
  Tooltip,
  CartesianGrid,
  ReferenceLine,
} from "recharts";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Tabs, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { Activity } from "lucide-react";

function CustomTooltip({ active, payload }) {
  if (active && payload && payload.length) {
    const item = payload[0].payload;
    const isPpm = payload[0].dataKey === "ppm";

    return (
      <div className="rounded-xl border border-zinc-200/90 bg-white/95 p-2.5 shadow-md backdrop-blur-md">
        <p className="text-[10px] font-semibold uppercase tracking-wider text-zinc-400">
          {item.timeLabel}
        </p>
        <div className="mt-1 flex items-baseline gap-1.5 border-b border-zinc-100 pb-1.5">
          <span className="text-base font-extrabold text-zinc-950">
            {typeof payload[0].value === "number"
              ? payload[0].value.toFixed(2)
              : payload[0].value}
          </span>
          <span className="text-[10px] font-medium text-zinc-500">
            {isPpm ? "PPM" : "ADC"}
          </span>
        </div>
        <div className="mt-1.5 flex gap-3 text-[10px] text-zinc-500">
          <span>Raw: {item.adc ?? 0}</span>
          <span>Fil: {item.filteredAdc ?? 0}</span>
        </div>
      </div>
    );
  }
  return null;
}

export default function H2SChart({ history = [] }) {
  const [metricKey, setMetricKey] = useState("ppm");

  // Format data chart dengan useMemo agar hemat re-render
  const formattedData = useMemo(() => {
    return history.map((item, index) => ({
      ...item,
      timeLabel: item.updatedAt
        ? new Date(item.updatedAt).toLocaleTimeString([], {
            hour: "2-digit",
            minute: "2-digit",
            second: "2-digit",
          })
        : `${index + 1}`,
    }));
  }, [history]);

  // Evaluasi batas tertinggi saat ini untuk styling dinamis
  const latestPpm = history.length > 0 ? history[history.length - 1].ppm : 0;
  const strokeColor = latestPpm >= 20 ? "#f43f5e" : latestPpm >= 10 ? "#f59e0b" : "#18181b";
  const stopColor = latestPpm >= 20 ? "#f43f5e" : latestPpm >= 10 ? "#f59e0b" : "#18181b";

  return (
    <Card className="flex h-full w-full flex-col justify-between overflow-hidden border-zinc-200/80 bg-white/95 shadow-2xs backdrop-blur-sm">
      {/* Header Compact */}
      <CardHeader className="flex shrink-0 flex-row items-center justify-between p-3 pb-1 sm:p-4 sm:pb-2">
        <div className="flex items-center gap-1.5">
          <Activity className="h-3.5 w-3.5 text-zinc-700 sm:h-4 sm:w-4" />
          <div>
            <CardTitle className="text-xs font-bold text-zinc-900 sm:text-sm">
              Tren Realtime
            </CardTitle>
            <p className="text-[10px] text-zinc-400 sm:block hidden">
              Buffer 60 titik terakhir
            </p>
          </div>
        </div>

        <Tabs value={metricKey} onValueChange={setMetricKey} className="w-auto">
          <TabsList className="h-7 bg-zinc-100/90 p-0.5 sm:h-8">
            <TabsTrigger
              value="ppm"
              className="px-2 py-0.5 text-[10px] font-semibold data-[state=active]:bg-white sm:text-xs"
            >
              PPM
            </TabsTrigger>
            <TabsTrigger
              value="filteredAdc"
              className="px-2 py-0.5 text-[10px] font-semibold data-[state=active]:bg-white sm:text-xs"
            >
              ADC
            </TabsTrigger>
          </TabsList>
        </Tabs>
      </CardHeader>

      {/* Chart Canvas Area */}
      <CardContent className="min-h-0 flex-1 p-2 pt-0 sm:p-4 sm:pt-0">
        {formattedData.length === 0 ? (
          <div className="flex h-full w-full items-center justify-center rounded-lg bg-zinc-50/80 text-[11px] text-zinc-400">
            Menunggu streaming telemetri...
          </div>
        ) : (
          <div className="h-full w-full">
            <ResponsiveContainer width="100%" height="100%">
              <AreaChart
                data={formattedData}
                margin={{ top: 8, right: 6, left: -24, bottom: 0 }}
              >
                <defs>
                  <linearGradient id="chartGradient" x1="0" y1="0" x2="0" y2="1">
                    <stop offset="5%" stopColor={stopColor} stopOpacity={0.18} />
                    <stop offset="95%" stopColor={stopColor} stopOpacity={0.0} />
                  </linearGradient>
                </defs>
                <CartesianGrid strokeDasharray="2 2" vertical={false} stroke="#f4f4f5" />
                <XAxis
                  dataKey="timeLabel"
                  tick={{ fontSize: 9, fill: "#a1a1aa" }}
                  axisLine={{ stroke: "#f4f4f5" }}
                  tickLine={false}
                  interval="preserveStartEnd"
                  minTickGap={20}
                />
                <YAxis
                  tick={{ fontSize: 9, fill: "#a1a1aa" }}
                  axisLine={false}
                  tickLine={false}
                  domain={metricKey === "ppm" ? [0, "auto"] : ["auto", "auto"]}
                />
                <Tooltip content={<CustomTooltip />} />
                {metricKey === "ppm" && (
                  <ReferenceLine
                    y={10}
                    stroke="#f59e0b"
                    strokeDasharray="3 3"
                    label={{
                      value: "OSHA Limit",
                      fill: "#d97706",
                      fontSize: 8,
                      position: "insideTopRight",
                    }}
                  />
                )}
                <Area
                  type="monotone"
                  dataKey={metricKey}
                  stroke={strokeColor}
                  strokeWidth={1.8}
                  fill="url(#chartGradient)"
                  isAnimationActive={false}
                />
              </AreaChart>
            </ResponsiveContainer>
          </div>
        )}
      </CardContent>
    </Card>
  );
}