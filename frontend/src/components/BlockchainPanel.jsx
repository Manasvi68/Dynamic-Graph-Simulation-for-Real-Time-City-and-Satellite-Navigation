import React, { useState } from 'react';
import { ChevronDown, ChevronRight, Shield, ShieldCheck } from 'lucide-react';

function BlockchainPanel({ blocks }) {
  const [expandedBlock, setExpandedBlock] = useState(null);

  if (!blocks || blocks.length === 0) {
    return <p className="text-gray-500 text-sm">No blockchain data.</p>;
  }

  const toggleBlock = (index) => {
    setExpandedBlock(expandedBlock === index ? null : index);
  };

  return (
    <div className="space-y-2 max-h-80 overflow-y-auto pr-1">
      {blocks.map((block) => {
        const isExpanded = expandedBlock === block.index;
        const isGenesis = block.index === 0;
        const eventData = typeof block.data === 'object' ? block.data : null;

        return (
          <div
            key={block.index}
            className="glass-panel rounded-lg overflow-hidden"
          >
            {/* block header */}
            <button
              onClick={() => toggleBlock(block.index)}
              className="w-full flex items-center gap-2 px-3 py-2 text-left hover:bg-white/5 transition-colors"
            >
              {isExpanded ? (
                <ChevronDown size={14} className="text-gray-400" />
              ) : (
                <ChevronRight size={14} className="text-gray-400" />
              )}

              {isGenesis ? (
                <Shield size={14} className="text-yellow-400" />
              ) : (
                <ShieldCheck size={14} className="text-emerald-400" />
              )}

              <span className="text-sm font-medium text-gray-200">
                Block #{block.index}
              </span>

              {eventData && (
                <span className="ml-auto text-xs text-gray-500">
                  {eventData.type}
                </span>
              )}

              {isGenesis && (
                <span className="ml-auto text-xs text-yellow-400/70">
                  Genesis
                </span>
              )}
            </button>

            {/* expanded details */}
            {isExpanded && (
              <div className="px-3 pb-3 text-xs space-y-1 border-t border-white/5 pt-2">
                <div className="font-mono text-gray-400">
                  <span className="text-gray-600">Hash: </span>
                  {block.hash}
                </div>
                <div className="font-mono text-gray-400">
                  <span className="text-gray-600">Prev: </span>
                  {block.previousHash}
                </div>

                {eventData && (
                  <div className="mt-2 p-2 rounded bg-black/30 space-y-1">
                    <div><span className="text-gray-500">Type:</span> <span className="text-cyan-300">{eventData.type}</span></div>
                    <div><span className="text-gray-500">From:</span> {eventData.from}</div>
                    <div><span className="text-gray-500">To:</span> {eventData.to}</div>
                    {eventData.oldWeight > 0 && (
                      <div><span className="text-gray-500">Weight:</span> {eventData.oldWeight} → {eventData.newWeight}</div>
                    )}
                  </div>
                )}

                {isGenesis && (
                  <div className="mt-2 p-2 rounded bg-black/30 text-yellow-400/70">
                    Genesis Block — chain origin
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
