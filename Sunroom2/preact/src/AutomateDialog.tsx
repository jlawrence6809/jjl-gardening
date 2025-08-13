import { useEffect, useState } from 'preact/hooks';
import { parseInputString, Err, ParsedRule } from './RuleParser';
import { FullScreenDialog } from './components/Dialog';
import { Relay } from './types';

type AutomateDialogProps = {
  relay: Relay | null;
  label: string | undefined;
  setLabel: (label: string) => void;
  onClose: (refreshRelays: boolean) => void;
};

export const AutomateDialog = ({
  relay,
  label,
  setLabel,
  onClose,
}: AutomateDialogProps) => {
  const [rule, setRule] = useState<string>('loading');
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
      onClose(true);
    } catch (error) {
      console.error(error);
      alert('Error submitting rule');
    }
  };

  return (
    <FullScreenDialog onClose={() => onClose(false)}>
      <div className="AutomateDialog">
        <h3>
          <span
            contentEditable
            onBlur={(ev) => setLabel(ev.currentTarget.textContent.trim())}
          >
            {label}
          </span>
          <sup className="Pencil">✏️</sup>
        </h3>
        <textarea
          value={rule}
          disabled={rule === 'loading'}
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
