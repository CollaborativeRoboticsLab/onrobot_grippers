# Software Follow-up

These software and repository tasks are kept separate from the hardware-only
acceptance checklist in [ToDO.md](../ToDO.md). Their status does not authorize
hardware writes or imply physical validation.

- [ ] Extend validation coverage for overflow, rounding, numeric boundaries,
  and NaN/Inf across the relevant codecs and goal validators.
- [ ] Review action-server state transitions, callback responsiveness, and
  simultaneous-goal behavior across each software backend.
- [ ] Keep command replay and software-stop semantics explicit and tested in
  offline tests; distinguish them from physical stop behavior.
- [ ] Prepare a review summary with change provenance, test results, supported
  software scope, and unresolved assumptions.
- [ ] Verify mesh/model asset provenance and accuracy before redistribution or
  making accuracy claims.