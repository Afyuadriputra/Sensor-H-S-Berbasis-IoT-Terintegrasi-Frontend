import React from "react";
import { motion, AnimatePresence } from "framer-motion";
import { Card, CardContent } from "@/components/ui/card";
import { cn } from "@/lib/utils";

export default function MetricCard({
  label,
  value,
  description,
  unit,
  icon: Icon,
  accentColor,
  className,
}) {
  return (
    <Card
      className={cn(
        "group relative overflow-hidden border-zinc-200/80 bg-white/95 shadow-2xs backdrop-blur-sm transition-all duration-300 hover:border-zinc-300 hover:shadow-xs",
        className
      )}
    >
      <CardContent className="p-3 sm:p-4">
        {/* Header: Label & Icon */}
        <div className="flex items-center justify-between gap-2">
          <p className="truncate text-[10px] font-semibold uppercase tracking-wider text-zinc-400 sm:text-xs">
            {label}
          </p>

          {Icon && (
            <div
              className={cn(
                "flex h-6 w-6 shrink-0 items-center justify-center rounded-md bg-zinc-100/90 text-zinc-600 transition-colors group-hover:bg-zinc-900 group-hover:text-white sm:h-7 sm:w-7",
                accentColor
              )}
            >
              <Icon className="h-3 w-3 sm:h-3.5 sm:w-3.5" />
            </div>
          )}
        </div>

        {/* Value with Live Micro-Animation */}
        <div className="mt-1.5 flex items-baseline gap-1 sm:mt-2">
          <AnimatePresence mode="popLayout" initial={false}>
            <motion.span
              key={String(value)}
              initial={{ opacity: 0.6, y: -2 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ duration: 0.18, ease: "easeOut" }}
              className="text-base font-bold tracking-tight text-zinc-950 sm:text-xl"
            >
              {value}
            </motion.span>
          </AnimatePresence>

          {unit && (
            <span className="text-[10px] font-medium text-zinc-400 sm:text-xs">
              {unit}
            </span>
          )}
        </div>

        {/* Description / Subtext */}
        {description && (
          <p className="mt-0.5 truncate text-[9px] font-medium text-zinc-500 sm:text-[11px]">
            {description}
          </p>
        )}
      </CardContent>
    </Card>
  );
}