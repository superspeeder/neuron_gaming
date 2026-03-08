# Example Module

An example of a runtime module for the engine which defines game logic.

Any module must contain a function exported with the name `neuron_module_entry` which has the signature
`void neuron_module_entry(neuron::api::IEngine *engine, neuron::api::ModuleLoadInfo load_info)`.

This function is called exactly once per module load. The entry point should return quickly (read: use this to link to engine callbacks or launch async tasks with the engine's async runtime).
**Do not spend forever in this function**. Long execution times may cause issues elsewhere.
