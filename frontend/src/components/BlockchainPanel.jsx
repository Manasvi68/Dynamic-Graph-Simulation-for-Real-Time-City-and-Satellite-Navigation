import React, { useState } from 'react';
import { ChevronDown, ChevronRight, Shield, ShieldCheck } from 'lucide-react';

const typeLabels = {
  normal: 'Normal',
  light_traffic: 'Light traffic',
  heavy_traffic: 'Heavy traffic',
  congestion: 'Congestion',
  accident: 'Accident',
  construction: 'Construction',
  closed: 'Road closed',
  CONGESTION: 'Traffic heavier',
  ROAD_CLOSED: 'Road closed',
  ROAD_UPDATE: 'Road updated',
  RECOVERY: 'Road recovered',
  SAT_LINK_UP: 'Satellite link added',
  SAT_LINK_DOWN: 'Satellite link lost',
};

function BlockchainPanel({ blocks }) {
  const [expandedBlock, setExpandedBlock] = useState(null);

  if (!blocks || blocks.length === 0) {
    return (
      <p className="rounded-lg border border-dashed border-white/25 bg-black/30 px-3 py-4 text-center text-sm font-medium leading-relaxed text-zinc-200">
        No events yet. Run a simulation step or edit a road — entries show up here.
      </p>
    );
  }

  const toggleBlock = (index) => {
    setExpandedBlock(expandedBlock === index ? null : index);
  };

  return (
    <div className="h-full max-h-full space-y-2 overflow-y-auto overflow-x-hidden pr-0.5">
      {blocks.map((block) => {
        const isExpanded = expandedBlock === block.index;
        const isGenesis = block.index === 0;
        const eventData = typeof block.data === 'object' ? block.data : null;
        const friendlyType = eventData?.type ? typeLabels[eventData.type] || eventData.type : null;

        return (
          <div
            key={block.index}
            className="overflow-hidden rounded-lg border border-white/15 bg-zinc-900/90 shadow-md"
          >
            <button
              type="button"
              onClick={() => toggleBlock(block.index)}
              className="flex w-full items-center gap-2 px-3 py-2.5 text-left transition-colors hover:bg-white/10"
            >
              {isExpanded ? (
                <ChevronDown size={16} className="shrink-0 text-zinc-300" />
              ) : (
                <ChevronRight size={16} className="shrink-0 text-zinc-300" />
              )}

              {isGenesis ? (
                <Shield size={16} className="shrink-0 text-amber-400" />
              ) : (
                <ShieldCheck size={16} className="shrink-0 text-emerald-400" />
              )}

              <span className="text-sm font-bold text-white">#{block.index}</span>

              {isGenesis && (
                <span className="ml-auto rounded-md bg-amber-500/25 px-2 py-0.5 text-xs font-bold text-amber-100">
                  Genesis
                </span>
              )}

              {!isGenesis && friendlyType && (
                <span className="ml-auto truncate text-xs font-semibold text-zinc-200" title={eventData.type}>
                  {friendlyType}
                </span>
              )}
            </button>

            {isExpanded && (
              <div className="space-y-2 border-t border-white/12 px-3 pb-3 pt-2 text-xs">
                <div className="break-all font-mono text-sm leading-relaxed text-zinc-200">
                  <span className="font-sans text-xs font-bold uppercase tracking-wide text-zinc-400">Hash </span>
                  {block.hash}
                </div>
                <div className="break-all font-mono text-sm leading-relaxed text-zinc-200">
                  <span className="font-sans text-xs font-bold uppercase tracking-wide text-zinc-400">Prev </span>
                  {block.previousHash}
                </div>

                {eventData && (
                  <div className="space-y-1.5 rounded-md border border-white/12 bg-black/40 p-3 text-sm">
                    <div className="text-zinc-100">
                      <span className="font-semibold text-zinc-400">Event </span>
                      <span className="font-bold text-cyan-300">{friendlyType || eventData.type}</span>
                    </div>
                    <div className="text-zinc-100">
                      <span className="font-semibold text-zinc-400">Route </span>
                      <span className="font-bold text-white">{eventData.from}</span>
                      <span className="text-zinc-500"> → </span>
                      <span className="font-bold text-white">{eventData.to}</span>
                    </div>
                    {eventData.oldWeight > 0 && (
                      <div className="text-zinc-100">
                        <span className="font-semibold text-zinc-400">Weight </span>
                        <span className="font-mono font-bold text-white">
                          {eventData.oldWeight} → {eventData.newWeight}
                        </span>
                      </div>
                    )}
                  </div>
                )}

                {isGenesis && (
                  <div className="rounded-md border border-amber-400/30 bg-amber-950/60 px-3 py-2 text-sm font-medium text-amber-100">
                    First block — anchors the chain so changes can be verified.
                  </div>
                )}
              </div>
            )}
          </div>
        );
      })}
    </div>
  );
}

export default BlockchainPanel;
