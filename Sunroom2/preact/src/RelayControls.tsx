import { useEffect, useState } from 'preact/hooks';
import { Section } from './components/Section';
import { AutomateDialog } from './AutomateDialog';
import {
  Relay,
  RelayStateValue,
  RelaySubmissionValue,
  RelaySubState,
} from './types';

/**
 * Sunroom
 */
// export const RELAY_COUNT = 8 as const;

/**
 * Barn
 */
export const RELAY_COUNT = 13 as const;

const RELAY_LIST = new Array(RELAY_COUNT)
  .fill(0)
  .map((_, i) => `relay_${i}` as Relay);

/**
 * Convert from the relay state submission value to the relay state value.
 */
const getRelayStateValues = (
  blob: Record<Relay, RelaySubmissionValue>,
): Record<Relay, RelayStateValue> => {
  return Object.fromEntries(
    Object.entries(blob).map(([key, rawValue]) => {
      const value = parseInt(rawValue);
      const onesDigit = (value % 10) as RelaySubState;
      const tensDigit = Math.floor(value / 10) as RelaySubState;
      return [key, { force: onesDigit, auto: tensDigit }];
    }),
  ) as Record<Relay, RelayStateValue>;
};

/**
 * This is the order:
 * force off -> force on -> force x -> force off -> etc...
 * So do not modify the tens digit, just the ones digit (force digit).
 */
const getNextRelayState = (current: RelayStateValue): RelayStateValue => {
  const { force, auto } = current;
  const nextForce = ((force + 1) % 3) as RelaySubState;
  return { force: nextForce, auto };
};

const getRelaySubmissionValue = (
  current: RelayStateValue,
): RelaySubmissionValue =>
  `${current.auto}${current.force}` as RelaySubmissionValue;

export const RelayControls = () => {
  const [relayState, setRelayState] = useState<
    Record<Relay, RelayStateValue | 'loading'>
  >(() =>
    RELAY_LIST.reduce((acc, relay) => ({ ...acc, [relay]: 'loading' }), {}),
  );

  const [relayLabels, setRelayLabels] = useState<Record<Relay, string>>(
    RELAY_LIST.reduce((acc, relay) => ({ ...acc, [relay]: '' }), {}),
  );

  const [automateDialogRelay, setAutomateDialogRelay] = useState<Relay | null>(
    null,
  );

  const isLoading = Object.values(relayState).some(
    (value) => value === 'loading',
  );

  const fetchRelays = async () => {
    const data = await fetch('/relays');
    const json = await data.json();
    setRelayState(getRelayStateValues(json));
  };

  useEffect(() => {
    fetchRelays();
  }, []);

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/relay-labels');
      const json = await data.json();
      setRelayLabels(json);
    };
    load();
  }, []);

  const updateRelayLabel = async (label: string) => {
    if (!automateDialogRelay) return;

    const oldLabel = relayLabels[automateDialogRelay];
    setRelayLabels({ ...relayLabels, [automateDialogRelay]: 'Updating...' });

    try {
      const formData = new FormData();
      formData.append('i', automateDialogRelay.split('_')[1]);
      formData.append('v', label);
      await fetch(`/relay-label`, {
        method: 'POST',
        body: formData,
      });
      setRelayLabels({ ...relayLabels, [automateDialogRelay]: label });
    } catch (error) {
      console.error(error);
      alert('Error updating relay label');
      setRelayLabels({ ...relayLabels, [automateDialogRelay]: oldLabel });
    }
  };

  const handleSubmit = async (relay: Relay, value: RelayStateValue) => {
    const previousState = relayState[relay];
    if (previousState === 'loading') return;
    setRelayState({
      ...relayState,
      [relay]: 'loading',
    });

    try {
      const data = new FormData();
      data.append(relay, getRelaySubmissionValue(value));
      const response = await fetch('/relays', {
        method: 'POST',
        body: data,
      });
      const json = await response.json();
      setRelayState(getRelayStateValues(json));
    } catch (_error) {
      setRelayState({
        ...relayState,
        [relay]: previousState,
      });
    }
  };

  return (
    <Section
      className={`RelayForm ${isLoading ? 'loading' : ''}`}
      title="Relay Controls"
    >
      {RELAY_LIST.map((relay) => {
        const value = relayState[relay];
        const label = relayLabels[relay];

        let stateClasses = 'loading';
        if (value !== 'loading') {
          stateClasses = `auto_${value.auto} force_${value.force}`;
        }

        return (
          <div
            key={relay}
            className={`ToggleSwitch ${stateClasses}`}
            onClick={() =>
              value !== 'loading' &&
              handleSubmit(relay, getNextRelayState(value))
            }
          >
            <div
              className={'AutomateButton'}
              onClick={(ev) => {
                ev.stopPropagation();
                setAutomateDialogRelay(relay);
              }}
            >
              ⛭
            </div>
            {label}
          </div>
        );
      })}
      <AutomateDialog
        key={automateDialogRelay}
        relay={automateDialogRelay}
        label={relayLabels[automateDialogRelay]}
        setLabel={updateRelayLabel}
        onClose={(refreshRelays) => {
          setAutomateDialogRelay(null);
          if (refreshRelays) fetchRelays();
        }}
      />
    </Section>
  );
};
