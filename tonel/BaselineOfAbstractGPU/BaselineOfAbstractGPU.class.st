Class {
	#name : #BaselineOfAbstractGPU,
	#superclass : #BaselineOf,
	#category : #BaselineOfAbstractGPU
}

{ #category : #baselines }
BaselineOfAbstractGPU >> baseline: spec [
    <baseline>
	spec for: #'common' do: [
		spec blessing: #'baseline'.
		spec postLoadDoIt: #postLoadActions.
		spec
			baseline: #'PhaNAPI' with: [
				spec
					repository: 'github://ronsaldo/phanapi/tonel' ].

		spec
			package: 'AbstractGPU-CorePharo' with: [
				spec requires: #(#'PhaNAPI' ). ];
			package: #'AbstractGPU-GeneratedPharo' with: [
				spec requires: #(#'AbstractGPU-CorePharo' ). ];
			package: #'AbstractGPU-Samples' with: [
				spec requires: #(#'AbstractGPU-Window' #'AbstractGPU-CorePharo' #'AbstractGPU-GeneratedPharo' ). ];
			package: #'AbstractGPU-Window' with: [
				spec requires: #(#'AbstractGPU-GeneratedPharo' #'AbstractGPU-CorePharo' ). ]. ].

]

{ #category : #baselines }
BaselineOfAbstractGPU >> postLoadActions [
	(Smalltalk globals at: #AGPUCBindings) fetchLibrary.
	(Smalltalk globals at: #AGPUGeneratedDoIt) initializeBindings.
]
