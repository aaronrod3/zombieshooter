// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSDragDropPayload.h"

UZSDragDropPayload* UZSDragDropPayload::Make(FGuid InstanceId, EZSDragSourceKind SourceKind)
{
	UZSDragDropPayload* Payload = NewObject<UZSDragDropPayload>();
	Payload->InstanceId = InstanceId;
	Payload->SourceKind = SourceKind;
	return Payload;
}
