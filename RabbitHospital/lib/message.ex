defmodule Message do
  @moduledoc false

  @enforce_keys [:attributes, :payload, :state]
  defstruct [:attributes, :payload, :state]

  @doc false
  def create(attributes, payload, state) do
    %__MODULE__{
      attributes: attributes,
      payload: payload,
      state: state
    }
  end
end
