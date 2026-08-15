import React from "react";
import { motion, AnimatePresence } from "framer-motion";
import { Radio, Cpu, Layers, Clock, ShieldAlert, Wifi, Activity } from "lucide-react";
import { Badge } from "@/components/ui/badge";
import { Card, CardContent } from "@/components/ui/card";

import H2SChart from "./components/H2SChart";
import { useH2S } from "./hooks/useH2S";

// Konfigurasi status, palet interaktif, dan ambient glow
function getStatusTheme(level, ppm) {
  if (level === 0 || ppm < 1) {
    return {
      label: "Aman / Normal",
      badgeClass: "bg-emerald-50 text-emerald-700 border-emerald-200",
      indicatorClass: "bg-emerald-500",
      progressColor: "bg-emerald-500",
      accentText: "text-emerald-600",
      borderGlow: "border-emerald-200/80 shadow-[0_0_25px_rgba(16,185,129,0.08)]",
      bgTint: "from-emerald-500/[0.04] to-transparent",
    };
  }
  if (level === 1 || ppm < 10) {
    return {
      label: "Waspada (Bau Terdeteksi)",
      badgeClass: "bg-amber-50 text-amber-700 border-amber-200",
      indicatorClass: "bg-amber-500",
      progressColor: "bg-amber-500",
      accentText: "text-amber-600",
      borderGlow: "border-amber-200/80 shadow-[0_0_25px_rgba(245,158,11,0.12)]",
      bgTint: "from-amber-500/[0.05] to-transparent",
    };
  }
  if (level === 2 || ppm < 20) {
    return {
      label: "Peringatan Bahaya",
      badgeClass: "bg-orange-50 text-orange-700 border-orange-200",
      indicatorClass: "bg-orange-500",
      progressColor: "bg-orange-500",
      accentText: "text-orange-600",
      borderGlow: "border-orange-200/80 shadow-[0_0_30px_rgba(249,115,22,0.15)]",
      bgTint: "from-orange-500/[0.07] to-transparent",
    };
  }
  return {
    label: "Kritis / Evakuasi",
    badgeClass: "bg-rose-50 text-rose-700 border-rose-200 animate-pulse",
    indicatorClass: "bg-rose-500",
    progressColor: "bg-rose-500",
    accentText: "text-rose-600",
    borderGlow: "border-rose-300 shadow-[0_0_35px_rgba(244,63,94,0.2)]",
    bgTint: "from-rose-500/[0.1] to-transparent",
  };
}

export default function App() {
  const { connected, data, history } = useH2S();
  const theme = getStatusTheme(data.level, data.ppm);

  const uptimeSeconds = Math.floor((data.uptimeMs || 0) / 1000);
  const formattedUptime =
    uptimeSeconds >= 60
      ? `${Math.floor(uptimeSeconds / 60)}m ${uptimeSeconds % 60}s`
      : `${uptimeSeconds}s`;

  return (
    <div className="relative flex h-dvh w-full flex-col justify-between overflow-hidden bg-[#fafafa] p-3 text-zinc-900 selection:bg-zinc-900 selection:text-white sm:p-6">
      
      {/* Dynamic Ambient Background Glow */}
      <div
        className={`pointer-events-none absolute inset-x-0 top-0 h-72 bg-gradient-to-b ${theme.bgTint} transition-colors duration-700 ease-in-out`}
      />

      <div className="relative mx-auto flex h-full w-full max-w-5xl flex-col justify-between gap-2.5">

        {/* 1. COMPACT APP HEADER */}
        <header className="flex shrink-0 items-center justify-between pt-1">
          <div>
            <div className="flex items-center gap-1.5">
              <span className={`h-2 w-2 rounded-full ${theme.indicatorClass} transition-colors duration-500`} />
              <span className="text-[10px] font-semibold uppercase tracking-wider text-zinc-400">
                TPA Muara Fajar
              </span>
            </div>
            <h1 className="text-lg font-bold tracking-tight text-zinc-950 sm:text-xl">
              H₂S Intelligent Monitor
            </h1>
          </div>

          <Badge
            variant="outline"
            className={`flex items-center gap-1.5 rounded-full px-2.5 py-0.5 text-[11px] font-medium transition-all ${
              connected
                ? "border-emerald-200 bg-white text-emerald-700 shadow-xs"
                : "border-zinc-200 bg-white text-zinc-500"
            }`}
          >
            <Wifi className={`h-3 w-3 ${connected ? "text-emerald-500 animate-pulse" : "text-zinc-400"}`} />
            {connected ? "Live MQTT" : "Connecting"}
          </Badge>
        </header>

        {/* 2. HERO METRIC CARD WITH DYNAMIC GLOW */}
        <section className="shrink-0">
          <Card className={`relative overflow-hidden bg-white/95 backdrop-blur-md transition-all duration-500 ${theme.borderGlow}`}>
            <CardContent className="p-4 sm:p-5">
              
              {/* Top Row: PPM Number & Status Badge */}
              <div className="flex items-center justify-between">
                <div>
                  <span className="text-[10px] font-semibold uppercase tracking-wider text-zinc-400">
                    Live Concentration
                  </span>
                  <div className="flex items-baseline gap-1.5">
                    <motion.span
                      key={data.ppm}
                      initial={{ opacity: 0.7, scale: 0.98 }}
                      animate={{ opacity: 1, scale: 1 }}
                      transition={{ duration: 0.2 }}
                      className="text-4xl font-extrabold tracking-tight text-zinc-950 sm:text-6xl"
                    >
                      {typeof data.ppm === "number" ? data.ppm.toFixed(2) : data.ppm}
                    </motion.span>
                    <span className="text-xs font-bold text-zinc-400 sm:text-sm">PPM</span>
                  </div>
                </div>

                <div className="flex flex-col items-end gap-1">
                  <Badge variant="outline" className={`rounded-full px-3 py-0.5 text-xs font-semibold ${theme.badgeClass}`}>
                    <span className={`mr-1.5 h-1.5 w-1.5 rounded-full ${theme.indicatorClass}`} />
                    {data.status || theme.label}
                  </Badge>
                  <span className="text-[10px] font-semibold text-zinc-400">
                    Level {data.level} of 3
                  </span>
                </div>
              </div>

              {/* Dynamic Safety Spectrum Bar */}
              <div className="mt-3">
                <div className="mb-1 flex justify-between text-[9px] font-medium text-zinc-400">
                  <span>0 PPM (Aman)</span>
                  <span>10 PPM (OSHA Limit)</span>
                  <span>20+ PPM</span>
                </div>
                <div className="relative h-1.5 w-full overflow-hidden rounded-full bg-zinc-100">
                  <div
                    className={`h-full transition-all duration-500 ease-out ${theme.progressColor}`}
                    style={{ width: `${Math.min(Math.max((data.ppm / 20) * 100, 4), 100)}%` }}
                  />
                </div>
              </div>

              {/* Explanation Note */}
              <p className="mt-2.5 truncate text-[11px] font-medium text-zinc-600 sm:text-xs">
                {data.effect || "Menerima data telemetri dari mikrokontroler ESP32..."}
              </p>

            </CardContent>
          </Card>
        </section>

        {/* 3. COMPACT METRICS GRID / HORIZONTAL STRIP */}
        <section className="grid shrink-0 grid-cols-3 gap-2">
          
          <div className="flex flex-col justify-center rounded-xl border border-zinc-200/80 bg-white p-2.5 shadow-2xs">
            <div className="flex items-center gap-1 text-[10px] font-medium text-zinc-400">
              <Layers className="h-3 w-3 text-zinc-500" />
              <span>ADC Fil.</span>
            </div>
            <span className="mt-0.5 text-sm font-bold text-zinc-900 sm:text-base">
              {data.filteredAdc || 0}
            </span>
            <span className="text-[9px] text-zinc-400 truncate">Raw: {data.adc || 0}</span>
          </div>

          <div className="flex flex-col justify-center rounded-xl border border-zinc-200/80 bg-white p-2.5 shadow-2xs">
            <div className="flex items-center gap-1 text-[10px] font-medium text-zinc-400">
              <Clock className="h-3 w-3 text-zinc-500" />
              <span>Uptime</span>
            </div>
            <span className="mt-0.5 text-sm font-bold text-zinc-900 sm:text-base">
              {formattedUptime}
            </span>
            <span className="text-[9px] text-zinc-400 truncate">ESP32-S2</span>
          </div>

          <div className="flex flex-col justify-center rounded-xl border border-zinc-200/80 bg-white p-2.5 shadow-2xs">
            <div className="flex items-center gap-1 text-[10px] font-medium text-zinc-400">
              <Activity className="h-3 w-3 text-zinc-500" />
              <span>Buffer</span>
            </div>
            <span className="mt-0.5 text-sm font-bold text-zinc-900 sm:text-base">
              {history.length}/60
            </span>
            <span className="text-[9px] text-zinc-400 truncate">Rolling data</span>
          </div>

        </section>

        {/* 4. REALTIME CHART (FLEX-GROW MENGISI SISA LAYAR) */}
        <section className="min-h-0 flex-1 pb-1">
          <H2SChart history={history} />
        </section>

      </div>
    </div>
  );
}