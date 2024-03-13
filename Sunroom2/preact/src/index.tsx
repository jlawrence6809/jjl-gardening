import './style';
import { useEffect, useState } from 'preact/hooks';
import { createPortal } from 'preact/compat';
import { VNode } from 'preact';
import { parseInputString, Err, ParsedRule } from './RuleParser';

const PORTAL_ROOT_ID = 'portal-root';

const RELAY_LABELS = {
  relay_0: 'Outlet A',
  relay_1: 'Outlet B',
  relay_2: 'Outlet C',
  relay_3: 'Outlet D',
  relay_4: 'NC',
  relay_5: 'Outdoor Lights',
  relay_6: 'Sunroom Lights',
  relay_7: 'Fan',
};

export default function App() {
  return (
    <>
      <div className="app-root">
        <h1>Sunroom 2</h1>
        <hr />
        <RelayControls />
        <hr />
        <GlobalInfo />
        <hr />
        <SensorInfo />
        <hr />
        <WifiForm />
        <hr />
        <Restart />
      </div>

      <div id={PORTAL_ROOT_ID}></div>
    </>
  );
}

type GlobalInfoResponse = {
  ChipId?: string;
  ResetCounter?: number;
  InternalTemperature?: number;
  CurrentTime?: string;
};

const GlobalInfo = () => {
  const [globalInfo, setGlobalInfo] = useState<GlobalInfoResponse>({});

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/global-info');
      const json = await data.json();
      setGlobalInfo(json);
    };
    load();
  }, []);

  return (
    <Section className="GlobalInfo" title="Global Info">
      <p>Chip Id: #{globalInfo.ChipId}</p>
      <p>Resets: {globalInfo.ResetCounter}</p>
      <p>Internal temperature: {globalInfo.InternalTemperature}F</p>
      <p>Current time: {globalInfo.CurrentTime}</p>
    </Section>
  );
};

type SensorInfoResponse = {
  Temperature?: number;
  Humidity?: number;
  Light?: number;
  Switch?: number;
};

const SensorInfo = () => {
  const [sensorInfo, setSensorInfo] = useState<SensorInfoResponse>({});

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/sensor-info');
      const json = await data.json();
      setSensorInfo(json);
    };
    load();
  }, []);

  return (
    <Section className="SensorInfo" title="Sensor Info">
      <p>Temperature: {sensorInfo.Temperature}F</p>
      <p>Humidity: {sensorInfo.Humidity}%</p>
      <p>Light: {sensorInfo.Light}</p>
      <p>Switch: {sensorInfo.Switch}</p>
    </Section>
  );
};

/*
 * 0 = off, 1 = on, 2 = x/dont care
 */
type RelaySubState = 0 | 1 | 2;

/**
 * Ones digit: force digit (0 = off, 1 = on, 2 = x/dont care)
 * Tens digit: auto digit (0 = off, 1 = on, 2 = x/dont care)
 */
type RelaySubmissionValue = `${RelaySubState}${RelaySubState}`;
type RelayStateValue = {
  force: RelaySubState;
  auto: RelaySubState;
};
type Relay =
  | 'relay_0'
  | 'relay_1'
  | 'relay_2'
  | 'relay_3'
  | 'relay_4'
  | 'relay_5'
  | 'relay_6'
  | 'relay_7';

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
  `${current.force}${current.auto}` as RelaySubmissionValue;

const RelayControls = () => {
  const [relayState, setRelayState] = useState<
    Record<Relay, RelayStateValue | 'loading'>
  >({
    relay_0: 'loading',
    relay_1: 'loading',
    relay_2: 'loading',
    relay_3: 'loading',
    relay_4: 'loading',
    relay_5: 'loading',
    relay_6: 'loading',
    relay_7: 'loading',
    // relay_0: { force: 0, auto: 0 },
    // relay_1: { force: 1, auto: 0 },
    // relay_2: { force: 2, auto: 0 },
    // relay_3: { force: 0, auto: 1 },
    // relay_4: { force: 1, auto: 1 },
    // relay_5: { force: 2, auto: 1 },
    // relay_6: { force: 0, auto: 2 },
    // relay_7: { force: 1, auto: 2 },
  });

  const [automateDialogRelay, setAutomateDialogRelay] = useState<Relay | null>(
    null,
  );

  const isLoading = Object.values(relayState).some(
    (value) => value === 'loading',
  );

  useEffect(() => {
    const load = async () => {
      const data = await fetch('/relays');
      const json = await data.json();
      setRelayState(getRelayStateValues(json));
    };
    load();
  }, []);

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
      {new Array(8).fill(0).map((_, i) => {
        const relay = `relay_${i}` as Relay;
        const value = relayState[relay];
        const label = RELAY_LABELS[relay];

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
              A
            </div>
            {label}
          </div>
        );
      })}
      <AutomateDialog
        relay={automateDialogRelay}
        onClose={() => setAutomateDialogRelay(null)}
      />
    </Section>
  );
};

type AutomateDialogProps = {
  relay: Relay | null;
  onClose: () => void;
};

const AutomateDialog = ({ relay, onClose }: AutomateDialogProps) => {
  // const [rule, setRule] = useState<Rule | null>(null);
  const [rule, setRule] = useState<string>('');
  const [validationResult, setValidationResult] = useState<ParsedRule | Err>(
    null,
  );
  const submitDisabled =
    !validationResult || (validationResult as Err)?.type === 'ERROR';

  const relayIdx = relay === null ? null : parseInt(relay?.split('_')?.[1]);

  useEffect(() => {
    const load = async () => {
      if (relayIdx === null) return;
      const data = await fetch(`/rule?i=${relayIdx}`);
      const json = await data.json();
      setRule(json.v);
    };
    load();
  }, [relayIdx]);

  if (relay === null) return <></>;

  const label = RELAY_LABELS[relay];

  const submit = async () => {
    if (submitDisabled) {
      return;
    }
    try {
      const formData = new FormData();
      formData.append('v', JSON.stringify(JSON.parse(rule)));
      formData.append('i', relayIdx.toString());

      await fetch(`/rule`, {
        method: 'POST',
        body: formData,
      });
      onClose();
    } catch (error) {
      console.error(error);
      alert('Error submitting rule');
    }
  };

  return (
    <FullScreenDialog onClose={onClose}>
      <div className="AutomateDialog">
        <h3>Automate {label}</h3>
        <textarea
          value={rule}
          onChange={(ev) => setRule(ev.currentTarget.value)}
          style={{ width: '100%', height: '200px' }}
        ></textarea>
        <div className="Buttons">
          <button onClick={() => setValidationResult(parseInputString(rule))}>
            Validate
          </button>
          <button onClick={() => submit()} disabled={submitDisabled}>
            Submit
          </button>
        </div>

        {/* display validation result */}
        {validationResult && (
          <div>
            <h4>Validation Result</h4>
            <pre>{JSON.stringify(validationResult, null, 2)}</pre>
          </div>
        )}

        {/* display current sensor actuator values */}
      </div>
    </FullScreenDialog>
  );
};

const WifiForm = () => {
  return (
    <Section title="Wifi Settings">
      <form className="WifiForm" action="/wifi-settings" method="post">
        <div>
          <label for="ssid">Wifi Name</label>
          <input type="text" id="ssid" name="ssid" />
        </div>
        <div>
          <label for="password">Wifi Password</label>
          <input type="password" id="password" name="password" />
        </div>
        <button type="submit">Submit</button>
      </form>
    </Section>
  );
};

const RESTART_SECONDS = 5;

const Restart = () => {
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

type SectionProps = {
  className?: string;
  title: string;
  children: any;
};

const Section = ({ className = '', title, children }: SectionProps) => {
  return (
    <div className={`Section ${className}`}>
      <h2>{title}</h2>
      {children}
    </div>
  );
};

type FullScreenDialogProps = {
  children: preact.ComponentChild;
  onClose: () => void;
};

const FullScreenDialog = ({ children, onClose }: FullScreenDialogProps) => {
  return (
    <DialogPortal>
      <div className="FullScreenDialog" onClick={onClose}>
        <div className="inner" onClick={(e) => e.stopPropagation()}>
          {children}
        </div>
      </div>
    </DialogPortal>
  );
};

const DialogPortal = ({ children }: { children: VNode<{}> }) => {
  const portalRoot = document.getElementById(PORTAL_ROOT_ID);
  return portalRoot ? createPortal(children, portalRoot) : null;
};
