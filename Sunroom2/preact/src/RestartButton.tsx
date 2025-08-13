import { useState } from 'preact/hooks';

const RESTART_SECONDS = 5;

export const RestartButton = () => {
  const [restartSuccess, setRestartSuccess] = useState(false);

  return (
    <div>
      <button
        onClick={async () => {
          try {
            await fetch('/reset', { method: 'POST' });
            setRestartSuccess(true);
            setTimeout(() => {
              setRestartSuccess(false);
            }, RESTART_SECONDS * 1000);
          } catch (error) {
            console.error(error);
            alert('Error restarting');
          }
        }}
      >
        Reset
      </button>
      {restartSuccess && (
        <p>Success, restarting in {RESTART_SECONDS} seconds...</p>
      )}
    </div>
  );
};
